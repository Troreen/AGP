#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "GameFramework/Runtime/GameApplication.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Runtime/IGame.h"
#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Rendering/WorldRenderer.h"
#include "GraphicsEngine/RHI/GraphicsCommandList.h"
#include "InputHandler.h"
#include "Timer.h"
#include "Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>

DECLARE_LOG_CATEGORY_WITH_NAME(LogGameFramework, GameFramework, Log);
DEFINE_LOG_CATEGORY(LogGameFramework);

namespace
{
	constexpr int KeyCount = 256;

	bool IsVirtualKeyDown(int key)
	{
		return (GetAsyncKeyState(key) & 0x8000) != 0;
	}

	LRESULT CALLBACK GameWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_CLOSE || message == WM_DESTROY)
		{
			PostQuitMessage(0);
		}
		return DefWindowProcW(window, message, wParam, lParam);
	}
}

// Platform and graphics details stay here. The gameplay path is the loop in Run.
class GameApplication::Impl
{
public:
	Impl(IGame& game, const Config& config, SceneSource source) : myGame(game), myConfig(config), mySource(std::move(source))
	{
	}

	~Impl()
	{
		if (myMainWindowHandle)
		{
			DestroyWindow(myMainWindowHandle);
		}
	}

	int Run();

private:
	void LoadPendingScene();
	GameInput CaptureInputFrame();
	void UpdateRenderPassTitle();
	void LogRuntimeStats() const;
	IGame& myGame;
	Config myConfig;
	SceneSource mySource;
	GameContext myContext;
	ComponentRegistry myRegistry;
	HWND myMainWindowHandle = nullptr;
	CommonUtilities::InputHandler myInputHandler;
	GraphicsCommandList myCommandList;
	GraphicsEngine::RenderSceneSnapshot mySnapshot;
	bool myHasMainThreadMouseLookAnchor = false;
	bool myStarted = false;
};

int GameApplication::Impl::Run()
{
	const wchar_t* className = L"AGPGameWindow";
	WNDCLASSW windowClass = {};
	windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = GameWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hInstance = GetModuleHandleW(nullptr);
	windowClass.lpszClassName = className;
	if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
	{
		throw std::runtime_error("Could not register game window");
	}
	myMainWindowHandle = CreateWindowW(className, myConfig.Title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, myConfig.Width,
	                                   myConfig.Height, nullptr, nullptr, windowClass.hInstance, nullptr);
	if (!myMainWindowHandle)
	{
		throw std::runtime_error("Could not create game window");
	}
	myContext.myContentRoot = std::filesystem::canonical(myConfig.ContentRoot);
	auto& graphics = GraphicsEngine::Get();
	if (!graphics.Initialize(myMainWindowHandle, myContext.myContentRoot / "Shaders") ||
	    !graphics.CreateCommandList("Game Scene", myCommandList))
	{
		throw std::runtime_error("Could not initialize game graphics");
	}
	myContext.myClientSize = graphics.GetClientSize();
	myInputHandler.SetWindowHandle(myMainWindowHandle);
	myInputHandler.SetAutoMouseCapture(false);
	myRegistry.RegisterBuiltIns();
	myGame.RegisterComponents(myRegistry);
	try
	{
		myGame.Initialize(myContext);
		if (myContext.myPendingScene)
		{
			LoadPendingScene();
		}
		else
		{
			myContext.GetWorld().BeginPlay();
		}
		myStarted = true;
		if (myConfig.ShowWindow)
		{
			ShowWindow(myMainWindowHandle, SW_SHOW);
			SetForegroundWindow(myMainWindowHandle);
		}
		if (myConfig.EnableRenderDiagnostics)
		{
			UpdateRenderPassTitle();
		}
		CommonUtilities::Timer timer;
		timer.Update();
		while (!myContext.myQuitRequested)
		{
			MSG message = {};
			while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				if (message.message == WM_QUIT)
				{
					myContext.RequestQuit();
				}
				myInputHandler.UpdateEvents(message.message, message.wParam, message.lParam);
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
			if (myContext.myQuitRequested)
			{
				break;
			}
			if (myContext.myPendingScene)
			{
				LoadPendingScene();
				// Loading time does not become a large movement delta.
				timer.Update();
			}
			timer.Update();
			myInputHandler.UpdateInput();
			myContext.myInput = CaptureInputFrame();
			const float elapsed = timer.GetDeltaTime();
			const float delta = std::isfinite(elapsed) ? std::clamp(elapsed, 0.f, .25f) : 0.f;

			myGame.Update(myContext, delta);
			myContext.GetWorld().Update(delta);
			WorldRenderer::Build(myContext.GetWorld(), graphics, mySnapshot);
			myCommandList.ResetCommandList();
			graphics.RenderSnapshot(myCommandList, mySnapshot);
			if (myCommandList.FinishCommandList())
			{
				graphics.ExecuteCommandList(myCommandList);
				graphics.Present();
			}
			if (myConfig.EnableRenderDiagnostics)
			{
				if (myContext.GetInput().IsKeyPressed(Keys::F6))
				{
					graphics.CycleRenderPass();
					UpdateRenderPassTitle();
				}
				if (myContext.GetInput().IsKeyPressed(Keys::P))
				{
					LogRuntimeStats();
				}
			}
		}
	}
	catch (...)
	{
		myContext.myAcceptSceneRequests = false;
		try
		{
			myGame.Shutdown(myContext);
		}
		catch (...)
		{
			LOG(LogGameFramework, Error, "Shutdown failed during exception cleanup");
		}
		myContext.GetWorld().Clear();
		throw;
	}
	myContext.myAcceptSceneRequests = false;
	myGame.Shutdown(myContext);
	myContext.GetWorld().Clear();
	return 0;
}

void GameApplication::Impl::LoadPendingScene()
{
	const std::string name = std::move(*myContext.myPendingScene);
	myContext.myPendingScene.reset();
	std::unique_ptr<World> world;
	try
	{
		if (!mySource)
		{
			throw std::runtime_error("No scene source installed");
		}
		AssetLibrary assets;
		SceneLoadContext context{myContext.myContentRoot, myContext.myClientSize, assets};
		const auto data = mySource(name, context);
		world = myRegistry.CreateWorld(data, assets, &myContext.myInput, myContext.myClientSize);
	}
	catch (const std::bad_alloc&)
	{
		throw;
	}
	catch (const std::exception& error)
	{
		LOG(LogGameFramework, Error, "Scene '{}': {}", name, error.what());
		myGame.OnSceneLoadFailed(myContext, name, error.what());
		if (!myStarted)
		{
			throw;
		}
		return; // Failed construction leaves the current scene intact.
	}
	myContext.myAcceptSceneRequests = false;
	myContext.myWorld->Clear();
	myContext.myWorld = std::move(world);
	myContext.mySceneName = name;
	myContext.myInput = {};
	myHasMainThreadMouseLookAnchor = false;
	myContext.myAcceptSceneRequests = true;
	myContext.GetWorld().BeginPlay();
	myGame.OnSceneLoaded(myContext, name);
}

int GameApplication::Run(IGame& game, const Config& config, SceneSource source)
{
	return Impl(game, config, std::move(source)).Run();
}

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

	LOG(LogGameFramework, Log,
	    "Deferred lists: opaque {}, blended {}. CPU ms: snapshot {:.3f}, prepare {:.3f}, shadows {:.3f} (wait {:.3f}), scene {:.3f}",
	    renderStats.OpaqueRenderItems, renderStats.BlendedRenderItems, renderStats.SnapshotMilliseconds,
	    renderStats.ResourcePreparationMilliseconds, renderStats.ShadowRecordingMilliseconds, renderStats.ShadowWaitMilliseconds,
	    renderStats.SceneRecordingMilliseconds);

	LOG(LogGameFramework, Log,
	    "Render stats: meshes visible {}/{}, shadow casters {}, lights relevant {}/{}, shadow passes D/S/P = {}/{}/{}",
	    renderStats.VisibleRenderItems, renderStats.TotalRenderItems, renderStats.ShadowCasters, renderStats.RelevantLights,
	    renderStats.TotalLights, renderStats.DirectionalShadowPasses, renderStats.SpotShadowPasses, renderStats.PointShadowPasses);
	LOG(LogGameFramework, Log, "Shadow culling/threading: caster draws {}, culled per pass {}, command lists recorded/executed {}/{}",
	    renderStats.ShadowCasterDraws, renderStats.CulledShadowCasters, renderStats.ShadowCommandListsRecorded,
	    renderStats.ShadowCommandListsExecuted);
}
