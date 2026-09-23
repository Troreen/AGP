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
#include "InputMapper.h"
#include "XInputHandler.h"
#include "EnumKeyCode.h"
#include "Timer.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace
{
	constexpr float MaxFrameDeltaSeconds = 0.25f;

	LRESULT CALLBACK GameWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_NCCREATE)
		{
			const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
			SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
		}
		auto* input = reinterpret_cast<CommonUtilities::InputHandler*>(GetWindowLongPtrW(window, GWLP_USERDATA));
		if (input)
		{
			// Includes sent messages such as WM_KILLFOCUS, not just queued input.
			input->UpdateEvents(message, wParam, lParam);
		}
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
	void RecenterMouseLook();
	void ShowRenderPassNotification();

	IGame& myGame;
	Config myConfig;
	SceneSource mySceneSource;
	GameContext myContext;
	ComponentRegistry myRegistry;

	HWND myMainWindowHandle = nullptr;

	CommonUtilities::InputHandler myInputHandler;
	CommonUtilities::XInputHandler myXInputHandler;
	std::vector<unsigned> myHostInputListenerIDs;

	GraphicsCommandList myCommandList;
	GraphicsEngine::RenderSceneSnapshot mySnapshot;
	std::shared_ptr<TextWidget> myRenderPassNotificationWidget;
	std::shared_ptr<FontAsset> myRenderFont;
	RenderPassNotificationTimer myRenderPassNotification;

	bool myToggleDebugCameraRequested = false;
	bool myInitialWorldStarted = false;

	DebugCameraService myDebugCamera;
};

int GameApplication::Impl::Run()
{
	bool gameStarted = false;
	bool shutdownAttempted = false;
	try
	{
		GraphicsEngine& graphics = InitializeWindowAndGraphics();
		InitializeServices();
		InitializeInputAndHostControls();
		gameStarted = true;
		StartGameSession();
		RunMainLoop(graphics);
		myContext.myAcceptSceneRequests = false;
		shutdownAttempted = true;
		myGame.Shutdown(myContext);
	}
	catch (...)
	{
		myContext.myAcceptSceneRequests = false;
		if (gameStarted && !shutdownAttempted)
		{
			try 
			{ 
				myGame.Shutdown(myContext); 
			}
			catch (...) 
			{ 
				LOG(LogGameFramework, Error, "Shutdown failed during exception cleanup"); 
			}
		}
		myContext.GetWorld().Clear();
		ShutdownServices();
		throw;
	}
	// All component and game listeners are removed before deleting the mapper.
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
	                                   myConfig.Height, nullptr, nullptr, windowClass.hInstance, &myInputHandler);
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
	ServiceLocator& services = ServiceLocator::GetInstance();
	AssetRegistry& assets = *services.SetAssetRegistry(new AssetRegistry());
	assets.Initialize(myContext.myContentRoot);
	if (!assets.IsInitialized())
	{
		throw std::runtime_error(assets.GetLastError());
	}

	// missing font does not prevent startup.
	if (myConfig.EnableRenderDiagnostics)
	{
		const std::shared_ptr<FontAsset> font = assets.GetAsset<FontAsset>("Fonts/CascadiaCode.font.json");
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

	AudioManager* audio = services.SetAudioManager(new AudioManager());
	audio->Init();

	services.SetInputMapper(new CommonUtilities::InputMapper());
}

void GameApplication::Impl::InitializeInputAndHostControls()
{
	myInputHandler.SetWindowHandle(myMainWindowHandle);
	myInputHandler.SetAutoMouseCapture(false);
	myInputHandler.SetMouseDeltaEnabled(myConfig.EnableMouseLook);
	auto& input = *ServiceLocator::GetInstance().GetInputMapper();
	input.Init(&myInputHandler, &myXInputHandler);
	input.BindActionToInputCode("DebugCamera", EKeyCode::F1);
	input.BindActionToInputCode("PreviousRenderPass", EKeyCode::F5);
	input.BindActionToInputCode("NextRenderPass", EKeyCode::F6);
	input.BindActionToInputCode("CameraLookEnable", EKeyCode::MOUSERBUTTON);
	input.BindActionToInputCode("CameraForward", EKeyCode::W);
	input.BindActionToInputCode("CameraBack", EKeyCode::S);
	input.BindActionToInputCode("CameraLeft", EKeyCode::A);
	input.BindActionToInputCode("CameraRight", EKeyCode::D);
	input.BindActionToInputCode("CameraUp", EKeyCode::SPACE);
	input.BindActionToInputCode("CameraDown", EKeyCode::CONTROL);
	input.BindActionToInputCode("CameraLookDelta", EPointerCode::MOUSE_DELTA);

	myHostInputListenerIDs.push_back(input.AddEventListener("DebugCamera", [this](const CommonUtilities::InputEvent& event)
	{
		if (event.inputData.isPressed)
		{
			myToggleDebugCameraRequested = true;
		}
	}));

	if (myConfig.EnableRenderDiagnostics)
	{
		myHostInputListenerIDs.push_back(input.AddEventListener("PreviousRenderPass", [this](const CommonUtilities::InputEvent& event)
		{
			if (event.inputData.isPressed)
			{
				GraphicsEngine::Get().SelectPreviousRenderPass();
				ShowRenderPassNotification();
			}
		}));
		myHostInputListenerIDs.push_back(input.AddEventListener("NextRenderPass", [this](const CommonUtilities::InputEvent& event)
		{
			if (event.inputData.isPressed)
			{
				GraphicsEngine::Get().SelectNextRenderPass();
				ShowRenderPassNotification();
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
		const float elapsed = timer.GetDeltaTime();
		float delta = 0.0f;
		if (CU::IsFinite(elapsed))
		{
			delta = CU::Clamp(elapsed, 0.0f, MaxFrameDeltaSeconds);
		}
		myRenderPassNotification.Update(delta);
		ServiceLocator::GetInstance().GetInputMapper()->Update();
		RecenterMouseLook();
		if (std::exchange(myToggleDebugCameraRequested, false))
		{
			myDebugCamera.Toggle(myContext.GetWorld(), myContext.myClientSize);
		}

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
	if (auto* input = ServiceLocator::GetInstance().GetInputMapper())
		for (unsigned id : myHostInputListenerIDs) input->RemoveEventListener(id);
	myHostInputListenerIDs.clear();
	myInputHandler.ReleaseMouse();
	ServiceLocator::GetInstance().KillServices();
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
		AssetRegistry& assets = ServiceLocator::GetInstance().GetAssetRegistry();
		SceneLoadContext loadContext{myContext.myContentRoot, myContext.myClientSize, assets};
		const SceneData scene = mySceneSource(*myContext.myPendingScene, loadContext);
		candidateWorld = myRegistry.CreateWorld(scene, assets, myContext.myClientSize);
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
	myToggleDebugCameraRequested = false;
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

std::shared_ptr<Font> GameApplication::GetFontResource(const std::shared_ptr<FontAsset>& asset)
{
	return asset->GetFont();
}

void GameApplication::Impl::RecenterMouseLook()
{
	// CenterMouse adjusts InputHandler's position baseline before warping the
	// cursor, so the warp never becomes a second source of action input.
	if (myConfig.EnableMouseLook && GetForegroundWindow() == myMainWindowHandle &&
		myInputHandler.IsKeyDown(int(EKeyCode::MOUSERBUTTON)))
	{
		myInputHandler.CenterMouse();
	}
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
