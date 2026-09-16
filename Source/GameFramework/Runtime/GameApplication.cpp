#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "GameApplication.h"
#include "GameContext.h"
#include "GameFramework/Runtime/Internal/GameLoop.h"
#include "IGame.h"
#include "GameFramework/Diagnostics/GameFrameworkLog.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/RHI/GraphicsCommandList.h"
#include "FrameScheduler.h"
#include "InputHandler.h"
#include "Timer.h"
#include "StartupOptions.h"
#include <condition_variable>
#include <cstring>
#include <exception>
#include <thread>

DECLARE_LOG_CATEGORY_WITH_NAME(LogRenderStats, RenderStats, Log);
DEFINE_LOG_CATEGORY(LogRenderStats);

namespace
{
	constexpr int KeyCount = 256;
	bool IsVirtualKeyDown(int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; }
	LRESULT CALLBACK GameWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_CLOSE || message == WM_DESTROY) PostQuitMessage(0);
		return DefWindowProcW(window, message, wParam, lParam);
	}
}

// Engine internals below this point. Gameplay code should not copy this machinery.
// The platform thread pumps input and renders snapshots; one gameplay owner runs
// all game/component hooks and builds snapshots. Initialize and Shutdown bracket
// that concurrent period so game objects never need their own locks for normal ticks.
struct GameApplication::Impl
{
	Impl(IGame& game, const Config& config) : myGame(game), myConfig(config), myLoop(config.FixedDeltaTime) {}
	~Impl()
	{
		Stop();
		myRenderSnapshots.ReleaseRendering();
		if (myMainWindowHandle) DestroyWindow(myMainWindowHandle);
	}
	int Run();
	void Advance(float delta, const GameInput& input);
	// Wake before joining: the worker may be asleep waiting for input. Joining must
	// finish before any state captured by its callbacks can be destroyed.
	void Stop()
	{
		myWorker.request_stop();
		myInputCondition.notify_all();
		if (myWorker.joinable()) myWorker.join();
	}
	GameInput CaptureInputFrame();
	void BuildAndPublishRenderSnapshot();
	void UpdateRenderPassTitle();
	void LogRuntimeStats() const;

	IGame& myGame;
	Config myConfig;
	GameContext myContext;
	GameFrameworkInternal::GameLoop myLoop;
	HWND myMainWindowHandle = nullptr;
	CommonUtilities::InputHandler myInputHandler;
	bool myHasMainThreadMouseLookAnchor = false;
	GraphicsCommandList myCommandList;
	// The renderer retains a completed snapshot while gameplay writes another slot.
	// World mutation does not wait for presentation; obsolete ready frames may be dropped.
	EngineScheduling::TripleBufferedSnapshotQueue<GraphicsEngine::RenderSceneSnapshot, 3> myRenderSnapshots;
	// Small input mailbox, not a lock around gameplay. The producer merges input/time
	// here; the worker copies it out and releases the lock before invoking game code.
	// The same lock protects worker failures until the main thread can rethrow them.
	std::mutex myInputMutex;
	std::condition_variable myInputCondition;
	GameInput myPendingInput;
	float myPendingDelta = 0;
	bool myHasPendingInput = false;
	std::exception_ptr myFailure;
	std::atomic<uint64_t> myTickCount = 0;
	std::jthread myWorker;
};

// Authoritative gameplay order: [game Fixed -> world Fixed] repeated as needed,
// then game Update -> all component Updates -> all component LateUpdates -> game Late.
// Only after that complete sequence may rendering receive a new world snapshot.
// This function is used unchanged by both runtime modes.
void GameApplication::Impl::Advance(float delta, const GameInput& input)
{
	myLoop.Advance(delta, input,
		[this](float dt, const GameInput& sample)
		{
			myContext.myInput = sample;
			myGame.FixedUpdate(myContext, dt);
			myContext.myWorld.FixedUpdate(dt);
			++myTickCount;
		},
		[this](float dt, const GameInput& sample)
		{
			myContext.myInput = sample;
			myGame.Update(myContext, dt);
			myContext.myWorld.Update(dt);
		},
		[this](float dt, const GameInput&)
		{
			myGame.LateUpdate(myContext, dt);
		});
	BuildAndPublishRenderSnapshot();
}

int GameApplication::Impl::Run()
{
	// --- Platform and graphics startup ---
	// Finish engine setup before handing control to the game. Shared asset creation
	// in Initialize is safe because the renderer/worker have not started consuming it.
	const wchar_t* className = L"AGPGameWindow";
	WNDCLASSW windowClass = {};
	windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = GameWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hInstance = GetModuleHandleW(nullptr);
	windowClass.lpszClassName = className;
	if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		throw std::runtime_error("Could not register game window");
	myMainWindowHandle = CreateWindowW(className, myConfig.Title.c_str(), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, myConfig.Width, myConfig.Height, nullptr, nullptr, windowClass.hInstance, nullptr);
	if (!myMainWindowHandle) throw std::runtime_error("Could not create game window");
	myContext.myContentRoot = std::filesystem::canonical(myConfig.ContentRoot);
	GraphicsEngine& graphics = GraphicsEngine::Get();
	if (!graphics.Initialize(myMainWindowHandle, myContext.myContentRoot / "Shaders") ||
		!graphics.CreateCommandList("Game Scene", myCommandList))
		throw std::runtime_error("Could not initialize game graphics");
	myContext.myClientSize = graphics.GetClientSize();
	myInputHandler.SetWindowHandle(myMainWindowHandle);
	myInputHandler.SetAutoMouseCapture(false);

	// Shutdown is called even after partial game initialization. No gameplay work outlives this scope.
	try
	{
		myGame.Initialize(myContext);
		BuildAndPublishRenderSnapshot();
		ShowWindow(myMainWindowHandle, SW_SHOW);
		SetForegroundWindow(myMainWindowHandle);
		if (myConfig.EnableRenderDiagnostics) UpdateRenderPassTitle();
		// --- Gameplay execution ---
		// Threading is an engine implementation choice. Games see the same serialized
		// callbacks regardless of this switch; a gameplay frame need not match a display frame.
		const bool threaded = myConfig.ThreadedUpdate && !StartupOptions::Disabled(L"AGP_DISABLE_THREADED_UPDATE");
		if (threaded)
		{
			myWorker = std::jthread([this](std::stop_token stop)
			{
				try
				{
					while (!stop.stop_requested())
					{
						GameInput input;
						float delta;
						{
							std::unique_lock lock(myInputMutex);
							myInputCondition.wait(lock, [&] { return stop.stop_requested() || myHasPendingInput; });
							if (stop.stop_requested()) break;
							input = myPendingInput;
							delta = myPendingDelta;
							myPendingInput = {};
							myPendingDelta = 0;
							myHasPendingInput = false;
						}
						Advance(delta, input);
					}
				}
				catch (...)
				{
					std::scoped_lock lock(myInputMutex);
					myFailure = std::current_exception();
				}
			});
		}
		// --- Platform frames and rendering ---
		// The main thread never reads live actor transforms while gameplay is running.
		// It exchanges copied input and completed snapshots instead.
		CommonUtilities::Timer timer;
		timer.Update();
		while (!myContext.myQuitRequested)
		{
			MSG message = {};
			bool quit = false;
			while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				if (message.message == WM_QUIT) { quit = true; break; }
				myInputHandler.UpdateEvents(message.message, message.wParam, message.lParam);
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
			if (quit) break;
			timer.Update();
			myInputHandler.UpdateInput();
			const GameInput input = CaptureInputFrame();
			if (myConfig.EnableRenderDiagnostics)
			{
				if (input.IsKeyPressed(Keys::F6)) { graphics.CycleRenderPass(); UpdateRenderPassTitle(); }
				if (input.IsKeyPressed(Keys::P)) LogRuntimeStats();
			}
			if (threaded)
			{
				{
					std::scoped_lock lock(myInputMutex);
					if (myFailure) std::rethrow_exception(myFailure);
					myPendingInput.Merge(input);
					myPendingDelta = (std::min)(myPendingDelta + timer.GetDeltaTime(), 0.25f);
					myHasPendingInput = true;
				}
				myInputCondition.notify_one();
			}
			else Advance(timer.GetDeltaTime(), input);
			// Render the newest completed state, or reuse the last one if gameplay is behind.
			// GPU command execution/presentation remain on this platform thread.
			myCommandList.ResetCommandList();
			if (const auto* snapshot = myRenderSnapshots.AcquireLatest())
			{
				graphics.RenderSnapshot(myCommandList, *snapshot);
				if (myCommandList.FinishCommandList())
				{
					graphics.ExecuteCommandList(myCommandList);
					graphics.Present();
				}
			}
			else
			{
				// A game may not select a camera until a later callback.
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		// --- Session teardown ---
		// Join even when exit was requested while a callback was running, then check for
		// a failure that may have arrived between the last main-thread check and shutdown.
		Stop();
		if (myFailure) std::rethrow_exception(myFailure);
	}
	catch (...)
	{
		Stop();
		const auto failure = std::current_exception();
		try { myGame.Shutdown(myContext); } catch (...) { GFLOG(Error, "Game Shutdown failed during exception cleanup"); }
		std::rethrow_exception(failure);
	}
	myGame.Shutdown(myContext);
	return 0;
}

int GameApplication::Run(IGame& game, const Config& config)
{
	Impl host(game, config);
	return host.Run();
}

// Platform input and cursor APIs stay here. Components receive copied values and
// never poll Win32 directly. Focus gating prevents ordinary background key input;
// the first mouse-look sample establishes an anchor instead of producing a jump.
GameInput GameApplication::Impl::CaptureInputFrame()
{
	GameInput inputFrame;
	const bool isFocused = myMainWindowHandle != nullptr && GetForegroundWindow() == myMainWindowHandle;

	for (int keyCode = 0; keyCode < KeyCount; ++keyCode)
	{
		inputFrame.KeysDown[static_cast<size_t>(keyCode)] = isFocused && (myInputHandler.IsKeyDown(keyCode) || IsVirtualKeyDown(keyCode));
		inputFrame.KeysPressed[static_cast<size_t>(keyCode)] = isFocused && myInputHandler.IsKeyPressed(keyCode);
	}

	const bool rightMouseDown = inputFrame.KeysDown[static_cast<size_t>(Keys::MOUSERBUTTON)];
	if (myConfig.EnableMouseLook && isFocused && rightMouseDown)
	{
		RECT clientRect = {};
		if (GetClientRect(myMainWindowHandle, &clientRect) != 0)
		{
			const POINT centerPoint = {(clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2};

			inputFrame.MouseLookActive = true;
			if (myHasMainThreadMouseLookAnchor)
			{
				POINT mousePosScreen = {};
				GetCursorPos(&mousePosScreen);
				POINT mousePosClient = mousePosScreen;
				ScreenToClient(myMainWindowHandle, &mousePosClient);
				inputFrame.MouseDeltaX = static_cast<float>(mousePosClient.x - centerPoint.x);
				inputFrame.MouseDeltaY = static_cast<float>(mousePosClient.y - centerPoint.y);
			}

			POINT centerPointScreen = centerPoint;
			ClientToScreen(myMainWindowHandle, &centerPointScreen);
			SetCursorPos(centerPointScreen.x, centerPointScreen.y);
			myHasMainThreadMouseLookAnchor = true;
		}
	}
	else
	{
		myHasMainThreadMouseLookAnchor = false;
	}

	return inputFrame;
}

// Copy gameplay values for rendering only after the update phases finish. Mesh and
// material references are shared rather than deep-copied: their underlying contents
// must stay stable during play. Failed extraction must return its buffer to the queue.
void GameApplication::Impl::BuildAndPublishRenderSnapshot()
{
	if (myContext.myCamera == nullptr)
	{
		return;
	}

	GraphicsEngine::RenderSceneSnapshot* snapshot = myRenderSnapshots.BeginBuild();
	if (snapshot == nullptr)
	{
		return;
	}

	try
	{
		if (GraphicsEngine::Get().BuildRenderSnapshot(*myContext.myCamera, myContext.myWorld, *snapshot))
			myRenderSnapshots.Publish(snapshot);
		else
			myRenderSnapshots.CancelBuild(snapshot);
	}
	catch (...)
	{
		myRenderSnapshots.CancelBuild(snapshot);
		throw;
	}
}

void GameApplication::Impl::UpdateRenderPassTitle()
{
	const char* passName = GraphicsEngine::Get().GetRenderPassName();
	const std::wstring widePassName(passName, passName + std::strlen(passName));
	const std::wstring title = myConfig.Title + L"  |  Render Pass: " + widePassName + L"  (F6 cycles)";
	SetWindowTextW(myMainWindowHandle, title.c_str());
}

void GameApplication::Impl::LogRuntimeStats() const
{
	const GraphicsEngine::RenderStats renderStats = GraphicsEngine::Get().GetLastRenderStats();
	const auto snapshotStats = myRenderSnapshots.GetStats();

	LOG(LogRenderStats, Log,
	    "Deferred lists: opaque {}, blended {}. CPU ms: snapshot {:.3f}, prepare {:.3f}, shadows {:.3f} (wait {:.3f}), scene {:.3f}",
	    renderStats.OpaqueRenderItems, renderStats.BlendedRenderItems, renderStats.SnapshotMilliseconds,
	    renderStats.ResourcePreparationMilliseconds, renderStats.ShadowRecordingMilliseconds, renderStats.ShadowWaitMilliseconds,
	    renderStats.SceneRecordingMilliseconds);

	LOG(LogRenderStats, Log, "Render stats: meshes visible {}/{}, shadow casters {}, lights relevant {}/{}, shadow passes D/S/P = {}/{}/{}",
	    renderStats.VisibleRenderItems, renderStats.TotalRenderItems, renderStats.ShadowCasters, renderStats.RelevantLights,
	    renderStats.TotalLights, renderStats.DirectionalShadowPasses, renderStats.SpotShadowPasses, renderStats.PointShadowPasses);
	LOG(LogRenderStats, Log, "Shadow culling/threading: caster draws {}, culled per pass {}, command lists recorded/executed {}/{}",
	    renderStats.ShadowCasterDraws, renderStats.CulledShadowCasters, renderStats.ShadowCommandListsRecorded,
	    renderStats.ShadowCommandListsExecuted);
	LOG(LogRenderStats, Log, "Snapshot worker: fixed ticks {}, published {}, reused previous {}, dropped ready {}",
	    myTickCount.load(), snapshotStats.PublishedSnapshots, snapshotStats.ReusedSnapshots,
	    snapshotStats.DroppedReadySnapshots);
}
