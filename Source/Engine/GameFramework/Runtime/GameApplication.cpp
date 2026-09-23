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
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/DebugCameraController.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/ServiceLocator.h"
#include "Maths.hpp"
#include "GraphicsEngine/RHI/GraphicsCommandList.h"
#include "GraphicsEngine/TextWidget.h"
#include "GameFramework/GameFrameworkLog.h"
#include "InputHandler.h"
#include "XInputHandler.h"
#include "EnumGamepadCode.h"
#include "EnumKeyCode.h"
#include "Timer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace
{
	constexpr int KeyCount = 256;
	constexpr float MaxFrameDeltaSeconds = 0.25f;
	constexpr std::array GamepadButtons{
		EGamepadCode::DPAD_UP, EGamepadCode::DPAD_DOWN, EGamepadCode::DPAD_LEFT, EGamepadCode::DPAD_RIGHT,
		EGamepadCode::BUTTON_START, EGamepadCode::BUTTON_BACK, EGamepadCode::THUMB_LEFT, EGamepadCode::THUMB_RIGHT,
		EGamepadCode::SHOULDER_LEFT, EGamepadCode::SHOULDER_RIGHT, EGamepadCode::BUTTON_A, EGamepadCode::BUTTON_B,
		EGamepadCode::BUTTON_X, EGamepadCode::BUTTON_Y};

	LRESULT CALLBACK GameWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_CLOSE || message == WM_DESTROY)
		{
			PostQuitMessage(0);
		}
		return DefWindowProcW(window, message, wParam, lParam);
	}
}

// Per-Run owner of platform state and the moved scene source. The game and any
// references captured by the source remain caller-owned. RunMainLoop shows frame order.
class GameApplication::Impl
{
public:
	Impl(IGame& game, const Config& config, SceneSource source) : myGame(game), myConfig(config), mySceneSource(std::move(source))
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
	GraphicsEngine& InitializeWindowAndGraphics();
	void InitializeServices();
	void InitializeInputAndHostControls();
	void StartGameSession();
	void RunMainLoop(GraphicsEngine& graphics);
	void PumpWindowMessages();
	// False skips a zero-sized frame; true permits rendering, with or without a resize.
	bool PrepareRenderTargetSize(GraphicsEngine& graphics);
	void RenderFrame(GraphicsEngine& graphics);
	void LoadPendingScene();
	void ShutdownServices();
	InputDeviceFrame CaptureInputFrame();
	// Uses the focus/button sample already populated by CaptureInputFrame.
	void CaptureMouseLookDeltaAndRecenterCursor(InputDeviceFrame& frame);
	void UpdateRenderPassTitle();
	void ShowRenderPassNotification();
	void LogRuntimeStats() const;
	IGame& myGame;
	Config myConfig;
	SceneSource mySceneSource;
	GameContext myContext;
	ComponentRegistry myRegistry;
	HWND myMainWindowHandle = nullptr;
	CommonUtilities::InputHandler myInputHandler;
	CommonUtilities::XInputHandler myXInputHandler;
	std::vector<InputSubscription> myHostInputSubscriptions;
	GraphicsCommandList myCommandList;
	GraphicsEngine::RenderSceneSnapshot mySnapshot;
	std::shared_ptr<TextWidget> myRenderPassNotificationWidget;
	FontHandle myRenderFont;
	RenderPassNotificationTimer myRenderPassNotification;
	bool myHasMouseLookAnchor = false;
	bool myInitialWorldStarted = false;
	DebugCameraService myDebugCamera;
};

int GameApplication::Impl::Run()
{
	GraphicsEngine& graphics = InitializeWindowAndGraphics();
	try
	{
		InitializeServices();
		InitializeInputAndHostControls();
		StartGameSession();
		RunMainLoop(graphics);
	}
	catch (...)
	{
		myContext.myAcceptSceneRequests = false;
		// Only Shutdown failures are caught here; later cleanup failures still escape.
		try
		{
			myGame.Shutdown(myContext);
		}
		catch (...)
		{
			LOG(LogGameFramework, Error, "Shutdown failed during exception cleanup");
		}
		myContext.GetWorld().Clear();
		ShutdownServices();
		throw;
	}
	// Normal Shutdown stays outside the catch: a failure skips explicit cleanup.
	myContext.myAcceptSceneRequests = false;
	myGame.Shutdown(myContext);
	myContext.GetWorld().Clear();
	ShutdownServices();
	return 0;
}

GraphicsEngine& GameApplication::Impl::InitializeWindowAndGraphics()
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

	GraphicsEngine& graphics = GraphicsEngine::Get();
	if (!graphics.Initialize(myMainWindowHandle, myContext.myContentRoot / "Shaders") ||
	    !graphics.CreateCommandList("Game Scene", myCommandList))
	{
		throw std::runtime_error("Could not initialize game graphics");
	}

	myContext.myClientSize = graphics.GetClientSize();
	return graphics;
}

void GameApplication::Impl::InitializeServices()
{
	AssetRegistry& assets = AssetRegistry::Get();
	assets.Initialize(myContext.myContentRoot);
	if (!assets.IsInitialized())
	{
		throw std::runtime_error(assets.GetLastError());
	}

	// Optional diagnostics overlay; a missing font does not prevent startup.
	if (myConfig.EnableRenderDiagnostics)
	{
		const FontHandle font = assets.ResolveFont(AssetId{"Fonts/CascadiaCode.font.json"});
		if (font)
		{
			myRenderFont = font;
			myRenderPassNotificationWidget = std::make_shared<TextWidget>();
			myRenderPassNotificationWidget->SetFont(GameApplication::GetFontResource(font));
			myRenderPassNotificationWidget->SetPosition({16.0f, 16.0f});
			myRenderPassNotificationWidget->SetPixelHeight(24.0f);
			myRenderPassNotificationWidget->SetColor(CU::Vector4f::One);
		}
		else
		{
			LOG(LogGameFramework, Error, "Render-pass overlay disabled: {}", assets.GetLastError());
		}
	}

	AudioManager* audio = AudioManager::GetInstance();
	audio->Init();

	ServiceLocator::GetInstance().ProvideInput(myContext.myInput);
	ServiceLocator::GetInstance().ProvideAudio(*audio);
	ServiceLocator::GetInstance().ProvideAssets(assets);
}

void GameApplication::Impl::InitializeInputAndHostControls()
{
	myInputHandler.SetWindowHandle(myMainWindowHandle);
	myInputHandler.SetAutoMouseCapture(false);
	InstallDefaultInputBindings(myContext.myInput);
	myHostInputSubscriptions.push_back(myContext.myInput.Subscribe(InputActions::ToggleTonemapping, [](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started)
		{
			GraphicsEngine::Get().ToggleTonemapping();
		}
	}));

	for (const auto [action, tonemapper] : {
		std::pair{&InputActions::SelectACES, Tonemapper::ACES},
		std::pair{&InputActions::SelectLottes, Tonemapper::Lottes},
		std::pair{&InputActions::SelectUnrealTonemapper, Tonemapper::UnrealEngine}})
	{
		myHostInputSubscriptions.push_back(myContext.myInput.Subscribe(*action, [tonemapper](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Started)
			{
				GraphicsEngine::Get().SetTonemapper(tonemapper);
			}
		}));
	}
	myHostInputSubscriptions.push_back(myContext.myInput.Subscribe(InputActions::DebugCamera, [this](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started)
		{
			myDebugCamera.Toggle(myContext.GetWorld(), myContext.myClientSize);
		}
	}));

	if (myConfig.EnableRenderDiagnostics)
	{
		myHostInputSubscriptions.push_back(myContext.myInput.Subscribe(InputActions::PreviousRenderPass, [this](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Started)
			{
				GraphicsEngine::Get().SelectPreviousRenderPass();
				UpdateRenderPassTitle();
				ShowRenderPassNotification();
			}
		}));
		myHostInputSubscriptions.push_back(myContext.myInput.Subscribe(InputActions::NextRenderPass, [this](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Started)
			{
				GraphicsEngine::Get().SelectNextRenderPass();
				UpdateRenderPassTitle();
				ShowRenderPassNotification();
			}
		}));
		myHostInputSubscriptions.push_back(myContext.myInput.Subscribe(InputActions::PrintDiagnostics, [this](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Started)
			{
				LogRuntimeStats();
			}
		}));
	}
}

void GameApplication::Impl::StartGameSession()
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
	myInitialWorldStarted = true;
	if (myConfig.ShowWindow)
	{
		ShowWindow(myMainWindowHandle, SW_SHOW);
		SetForegroundWindow(myMainWindowHandle);
	}
	if (myConfig.EnableRenderDiagnostics)
	{
		UpdateRenderPassTitle();
	}
}

void GameApplication::Impl::RunMainLoop(GraphicsEngine& graphics)
{
	CommonUtilities::Timer timer;
	timer.Update();
	while (!myContext.myQuitRequested)
	{
		PumpWindowMessages();
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

		if (!PrepareRenderTargetSize(graphics))
		{
			continue;
		}

		timer.Update();
		myInputHandler.UpdateInput();
		const float elapsed = timer.GetDeltaTime();
		float delta = 0.0f;
		if (CU::IsFinite(elapsed))
		{
			delta = CU::Clamp(elapsed, 0.0f, MaxFrameDeltaSeconds);
		}
		myRenderPassNotification.Update(delta);
		myContext.myInput.Update(CaptureInputFrame());

		myGame.Update(myContext, delta);
		myContext.GetWorld().Update(delta);
		ServiceLocator::GetInstance().GetAudioManager().Update(delta);
		RenderFrame(graphics);
	}
}

void GameApplication::Impl::PumpWindowMessages()
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
}

bool GameApplication::Impl::PrepareRenderTargetSize(GraphicsEngine& graphics)
{
	const CU::Vector2u clientSize = graphics.GetClientSize();
	if (clientSize.x == 0 || clientSize.y == 0)
	{
		return false;
	}
	if (clientSize.x != myContext.myClientSize.x || clientSize.y != myContext.myClientSize.y)
	{
		myCommandList.ResetCommandList();
		if (!graphics.Resize(clientSize.x, clientSize.y))
		{
			throw std::runtime_error("Failed to resize rendering targets");
		}
		myContext.myClientSize = clientSize;
	}
	return true;
}

void GameApplication::Impl::RenderFrame(GraphicsEngine& graphics)
{
	WorldRenderer::Build(myContext.GetWorld(), graphics, mySnapshot);
	if (myRenderPassNotificationWidget && myRenderPassNotification.IsVisible())
	{
		myRenderPassNotificationWidget->SetOpacity(myRenderPassNotification.GetOpacity());
		mySnapshot.ScreenTextItems.push_back(myRenderPassNotificationWidget);
	}
	myCommandList.ResetCommandList();
	graphics.RenderSnapshot(myCommandList, mySnapshot);
	if (myCommandList.FinishCommandList())
	{
		graphics.ExecuteCommandList(myCommandList);
		graphics.Present();
	}
}

void GameApplication::Impl::ShutdownServices()
{
	ServiceLocator::GetInstance().Clear();
	AudioManager::Shutdown();
	AssetRegistry::Get().Clear();
}

void GameApplication::Impl::LoadPendingScene()
{
	const std::string sceneName = GetSceneName(*myContext.myPendingScene);
	std::unique_ptr<World> candidateWorld;

	// Only candidate construction/configuration is recoverable; the live world stays in place.
	try
	{
		if (!mySceneSource)
		{
			throw std::runtime_error("No scene source installed");
		}
		AssetRegistry& assets = AssetRegistry::Get();
		SceneLoadContext loadContext{myContext.myContentRoot, myContext.myClientSize, assets};
		const SceneData scene = mySceneSource(*myContext.myPendingScene, loadContext);
		candidateWorld = myRegistry.CreateWorld(scene, assets, &myContext.myInput, myContext.myClientSize);
		myGame.ConfigureWorld(*candidateWorld);
	}
	catch (const std::bad_alloc&)
	{
		throw;
	}
	catch (const std::exception& error)
	{
		LOG(LogGameFramework, Error, "Scene '{}': {}", sceneName, error.what());
		myGame.OnSceneLoadFailed(myContext, sceneName, error.what());
		// Initial startup must succeed; later failures retain the live world and request.
		if (!myInitialWorldStarted)
		{
			throw;
		}
		return; // The retained request is retried at a later frame boundary.
	}

	// Replacement and activation failures escape to Run's exception cleanup.
	myContext.myAcceptSceneRequests = false;
	myContext.myWorld->Clear();
	myContext.myWorld = std::move(candidateWorld);
	// Construction callbacks may have overwritten the pending slot.
	myContext.myCurrentSceneType = *myContext.myPendingScene;
	myContext.myInput.Reset();
	myHasMouseLookAnchor = false;
	myDebugCamera.Reset();
	myContext.myAcceptSceneRequests = true;
	if (!myContext.GetWorld().GetActiveCamera())
	{
		myContext.GetWorld().SetActiveCamera(myDebugCamera.Ensure(myContext.GetWorld(), myContext.myClientSize));
	}
	myContext.GetWorld().BeginPlay();
	myGame.OnSceneLoaded(myContext, sceneName);
	// This also discards requests made by BeginPlay or OnSceneLoaded above.
	myContext.myPendingScene.reset();
}

int GameApplication::Run(IGame& game, const Config& config, SceneSource source)
{
	return Impl(game, config, std::move(source)).Run();
}

std::shared_ptr<Font> GameApplication::GetFontResource(const FontHandle& asset)
{
	return asset.myResource;
}

InputDeviceFrame GameApplication::Impl::CaptureInputFrame()
{
	InputDeviceFrame inputFrame;
	const bool isFocused = myMainWindowHandle != nullptr && GetForegroundWindow() == myMainWindowHandle;
	inputFrame.Focused = isFocused;

	for (int keyCode = 0; keyCode < KeyCount; ++keyCode)
	{
		inputFrame.KeysDown[static_cast<size_t>(keyCode)] = isFocused && (myInputHandler.IsKeyDown(keyCode));
	}

	CaptureMouseLookDeltaAndRecenterCursor(inputFrame);
	if (myXInputHandler.UpdateInput())
	{
		for (const EGamepadCode button : GamepadButtons)
		{
			const unsigned buttonCode = static_cast<unsigned>(button);
			inputFrame.GamepadButtonsDown[buttonCode] = myXInputHandler.IsButtonDown(buttonCode);
		}
		myXInputHandler.GetAnalogLeftValue(inputFrame.GamepadLeft);
		myXInputHandler.GetAnalogRightValue(inputFrame.GamepadRight);
		inputFrame.GamepadLeftTrigger = myXInputHandler.GetTriggerLeftValue();
		inputFrame.GamepadRightTrigger = myXInputHandler.GetTriggerRightValue();
	}

	return inputFrame;
}

void GameApplication::Impl::CaptureMouseLookDeltaAndRecenterCursor(InputDeviceFrame& frame)
{
	const bool rightMouseDown = frame.KeysDown[static_cast<size_t>(EKeyCode::MOUSERBUTTON)];
	if (myConfig.EnableMouseLook && frame.Focused && rightMouseDown)
	{
		RECT clientRect = {};
		if (GetClientRect(myMainWindowHandle, &clientRect) != 0)
		{
			const POINT centerPoint = {(clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2};

			if (myHasMouseLookAnchor)
			{
				POINT mousePosScreen = {};
				GetCursorPos(&mousePosScreen);
				POINT mousePosClient = mousePosScreen;
				ScreenToClient(myMainWindowHandle, &mousePosClient);
				frame.MouseDelta.x = static_cast<float>(mousePosClient.x - centerPoint.x);
				frame.MouseDelta.y = static_cast<float>(mousePosClient.y - centerPoint.y);
			}

			POINT centerPointScreen = centerPoint;
			ClientToScreen(myMainWindowHandle, &centerPointScreen);
			SetCursorPos(centerPointScreen.x, centerPointScreen.y);
			myHasMouseLookAnchor = true;
		}
	}
	else
	{
		myHasMouseLookAnchor = false;
	}
}

void GameApplication::Impl::UpdateRenderPassTitle()
{
	const char* passName = GraphicsEngine::Get().GetRenderPassName();
	const std::wstring widePassName(passName, passName + std::strlen(passName));
	const std::wstring title = myConfig.Title + L"  |  Render Pass: " + widePassName + L"  (F5 previous, F6 next)";
	SetWindowTextW(myMainWindowHandle, title.c_str());
}

void GameApplication::Impl::ShowRenderPassNotification()
{
	if (!myRenderPassNotificationWidget)
	{
		return;
	}
	myRenderPassNotificationWidget->SetText(std::string("Render Pass: ") + GraphicsEngine::Get().GetRenderPassName());
	myRenderPassNotificationWidget->SetOpacity(1.0f);
	myRenderPassNotification.Restart();
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
	LOG(LogGameFramework, Log, "Overlay stats: text draws {}, glyphs {}", renderStats.TextDrawCalls, renderStats.RenderedGlyphs);
}
