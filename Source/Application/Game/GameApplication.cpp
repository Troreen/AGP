#include "GameApplication.h"

#include "Game.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/FontAsset.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GameFramework/Rendering/WorldRenderer.h"
#include "GameFramework/Scenes/WorldFromSceneData.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GameFramework/World/World.h"
#include "GraphicsEngine/TextWidget.h"
#include "EnumKeyCode.h"
#include "InputMapper.h"
#include "Maths.hpp"
#include "PrimitiveMeshBuilder.h"
#include "Timer.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace
{
	constexpr float MaxFrameDeltaSeconds = 0.25f;
	constexpr float RenderPassNotificationDurationSeconds = 2.0f;
	constexpr float RenderPassNotificationFadeDurationSeconds = 0.5f;

	struct GameSceneDefinition
	{
		std::string_view DisplayName;
		std::filesystem::path RelativeFile;
	};

	GameSceneDefinition GetGameSceneDefinition(SceneId aSceneId)
	{
		switch (aSceneId)
		{
		case SceneId::Blockout:
			return {"Lvl_Blockout_Level", "ExportedScenes/lvl_blockout/Lvl_Blockout_Level.json"};
		case SceneId::Chests:
			return {"TestExportMap", "ExportedScenes/TestExportMap_Level.json"};
		case SceneId::ChestMaterials:
			return {"ChestMaterials", "ExportedScenes/ChestMaterials_Level.json"};
		}
		throw std::runtime_error("Unknown game scene id");
	}

	LRESULT CALLBACK GameWindowProc(HWND aWindow, UINT aMessage, WPARAM aWParam, LPARAM anLParam)
	{
		if (aMessage == WM_NCCREATE)
		{
			const auto* create = reinterpret_cast<const CREATESTRUCTW*>(anLParam);
			SetWindowLongPtrW(aWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
		}
		auto* input = reinterpret_cast<CommonUtilities::InputHandler*>(GetWindowLongPtrW(aWindow, GWLP_USERDATA));
		if (input)
		{
			input->UpdateEvents(aMessage, aWParam, anLParam);
		}
		if (aMessage == WM_CLOSE || aMessage == WM_DESTROY)
		{
			PostQuitMessage(0);
		}
		return DefWindowProcW(aWindow, aMessage, aWParam, anLParam);
	}

	// Try one cleanup step without stopping the remaining steps if it fails.
	template <class Action, class... Arguments>
	void AttemptCleanup(std::exception_ptr& aPrimaryFailure, const char* aStep, Action anAction, Arguments&&... someArguments)
	{
		try
		{
			std::invoke(anAction, std::forward<Arguments>(someArguments)...);
		}
		catch (const std::exception& error)
		{
			// Keep the first failure for Run to report; log any later ones.
			if (!aPrimaryFailure)
			{
				aPrimaryFailure = std::current_exception();
			}
			else
			{
				LOG(LogGameFramework, Error, "{} failed during cleanup: {}", aStep, error.what());
			}
		}
		catch (...)
		{
			if (!aPrimaryFailure)
			{
				aPrimaryFailure = std::current_exception();
			}
			else
			{
				LOG(LogGameFramework, Error, "{} failed during cleanup", aStep);
			}
		}
	}
}

GameApplication::GameApplication(Config aConfig) : myConfig(std::move(aConfig))
{
}

GameApplication::~GameApplication()
{
	DestroyWindowIfCreated();
}

int GameApplication::Run(Game& aGame)
{
	std::exception_ptr failure;
	try
	{
		RunSession(aGame);
	}
	catch (...)
	{
		failure = std::current_exception();
	}

	// Clean up after normal exit or failure, preserving the first exception.
	Cleanup(aGame, failure);

	// Report the failure only after all cleanup steps have been attempted.
	if (failure)
	{
		std::rethrow_exception(failure);
	}
	return 0;
}

void GameApplication::RunSession(Game& aGame)
{
	GraphicsEngine& graphics = InitializeWindowAndGraphics();
	InitializeServices();
	InitializeInputAndApplicationControls();
	myGameInitializationStarted = true;
	StartGameSession(aGame);
	RunMainLoop(aGame, graphics);
}

bool GameApplication::RequestSceneLoad(SceneId aSceneId)
{
	if (!myAcceptSceneRequests)
	{
		return false;
	}
	// pendingScene is consumed in the next frame by the update loop
	myPendingSceneId = aSceneId;
	return true;
}

// TODO: this probably should be moved to a sceneloader class or similar, since it is not really a GameApplication concern.
bool GameApplication::ReloadCurrentScene()
{
	return myCurrentSceneId && RequestSceneLoad(*myCurrentSceneId);
}

GraphicsEngine& GameApplication::InitializeWindowAndGraphics()
{
	// TODO: read these from config instead of hardcoding them
	// Window class and creation
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

	myMainWindowHandle = CreateWindowW(
		className, myConfig.Title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		myConfig.Width, myConfig.Height, nullptr, nullptr, windowClass.hInstance, &myInputHandler);

	if (!myMainWindowHandle)
	{
		throw std::runtime_error("Could not create game window");
	}

	// Initialize graphics engine
	myContentRoot = std::filesystem::canonical(myConfig.ContentRoot);
	GraphicsEngine& graphics = GraphicsEngine::Get();
	if (!graphics.Initialize(myMainWindowHandle, myContentRoot / "Shaders") || 
		!graphics.CreateCommandList("Game Scene", myCommandList))
	{
		throw std::runtime_error("Could not initialize graphics engine: ");
	}
	myClientSize = graphics.GetClientSize();
	return graphics;
}

void GameApplication::InitializeServices()
{
	ServiceLocator& services = ServiceLocator::GetInstance();
	AssetRegistry& assets = *services.SetAssetRegistry(new AssetRegistry());
	assets.Initialize(myContentRoot);
	if (!assets.IsInitialized())
	{
		throw std::runtime_error(assets.GetLastError());
	}

	// TODO: does this belong here?
	if (myConfig.EnableRenderDiagnostics)
	{
		const std::shared_ptr<FontAsset> font = assets.GetAsset<FontAsset>("Fonts/CascadiaCode.font.json");
		if (font)
		{
			myRenderFont = font;
			myRenderPassNotificationWidget = std::make_shared<TextWidget>();
			myRenderPassNotificationWidget->SetFont(font->GetFont());
			myRenderPassNotificationWidget->SetPosition({16.0f, 16.0f});
			myRenderPassNotificationWidget->SetPixelHeight(24.0f);
			myRenderPassNotificationWidget->SetColor(CU::Vector4f::One);
		}
		else
		{
			LOG(LogGameFramework, Log, "Render-pass overlay disabled: {}", assets.GetLastError());
		}
	}

	AudioManager* audio = services.SetAudioManager(new AudioManager());
	audio->Init();
	services.SetInputMapper(new CommonUtilities::InputMapper());
}

void GameApplication::InitializeInputAndApplicationControls()
{
	myInputHandler.SetWindowHandle(myMainWindowHandle);
	myInputHandler.SetAutoMouseCapture(false);
	myInputHandler.SetMouseDeltaEnabled(myConfig.EnableMouseLook);
	auto& input = *ServiceLocator::GetInstance().GetInputMapper();
	input.Init(&myInputHandler, &myXInputHandler);
	input.BindActionToInputCode("Quit", EKeyCode::ESCAPE);
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

	myApplicationInputListenerIds.push_back(input.AddEventListener("Quit", [this](const CommonUtilities::InputEvent& anEvent)
	{
		if (anEvent.inputData.isPressed)
		{
			myQuitRequested = true;
		}
	}));
	myApplicationInputListenerIds.push_back(input.AddEventListener("DebugCamera", [this](const CommonUtilities::InputEvent& anEvent)
	{
		if (anEvent.inputData.isPressed)
		{
			myToggleDebugCameraRequested = true;
		}
	}));

	if (myConfig.EnableRenderDiagnostics)
	{
		myApplicationInputListenerIds.push_back(input.AddEventListener("PreviousRenderPass", [this](const CommonUtilities::InputEvent& anEvent)
		{
			if (anEvent.inputData.isPressed)
			{
				GraphicsEngine::Get().SelectPreviousRenderPass();
				ShowRenderPassNotification();
			}
		}));
		myApplicationInputListenerIds.push_back(input.AddEventListener("NextRenderPass", [this](const CommonUtilities::InputEvent& anEvent)
		{
			if (anEvent.inputData.isPressed)
			{
				GraphicsEngine::Get().SelectNextRenderPass();
				ShowRenderPassNotification();
			}
		}));
	}
}

void GameApplication::StartGameSession(Game& aGame)
{
	aGame.Initialize(*this);
	if (!myPendingSceneId)
	{
		throw std::runtime_error("Game initialization did not request an initial scene");
	}

	ProcessPendingSceneLoad(aGame);
	
	myInitialWorldStarted = true;
	
	ShowWindow(myMainWindowHandle, SW_SHOW);
	SetForegroundWindow(myMainWindowHandle);
}

void GameApplication::RunMainLoop(Game& aGame, GraphicsEngine& aGraphics)
{
	// Initialise timer
	CommonUtilities::Timer timer;
	timer.Update();

	// Main loop
	while (!myQuitRequested)
	{
		PumpWindowMessages();
		if (myQuitRequested)
		{
			break;
		}

		// Process any pending scene load requests before ticking the timer, so that the first frame of a new scene is not delayed by a full frame time.
		if (myPendingSceneId)
		{
			ProcessPendingSceneLoad(aGame);
			timer.Update();
		}

		// skip updating and rendering if the window is minimized or has no client area
		if (!PrepareRenderTargetSize(aGraphics))
		{
			continue;
		}

		// Tick
		timer.Update();
		const float elapsed = timer.GetDeltaTime();
		const float delta = CommonUtilities::IsFinite(elapsed) ? CommonUtilities::Clamp(elapsed, 0.0f, MaxFrameDeltaSeconds) : 0.0f;
		
		myRenderPassNotificationRemainingSeconds = (std::max)(0.0f, myRenderPassNotificationRemainingSeconds - delta);

		// Update input
		ServiceLocator::GetInstance().GetInputMapper()->Update();
		
		if (myQuitRequested)
		{
			break;
		}

		// Handle debug camera toggle and mouse look
		RecenterMouseLook();
		if (std::exchange(myToggleDebugCameraRequested, false))
		{
			myDebugCamera.Toggle(*myWorld, myClientSize);
		}

		// Update game and world
		aGame.Update(*myWorld, delta);
		myWorld->Update(delta);
		ServiceLocator::GetInstance().GetAudioManager().Update(delta);

		RenderFrame(aGraphics);
	}
}

void GameApplication::PumpWindowMessages()
{
	MSG message = {};
	while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_QUIT)
		{
			myQuitRequested = true;
		}
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}

bool GameApplication::PrepareRenderTargetSize(GraphicsEngine& aGraphics)
{
	// we are supposed to disable window resizing according to spec but this gives us more options and can exist for now and be configured later
	const CU::Vector2u clientSize = aGraphics.GetClientSize();
	if (clientSize.x == 0 || clientSize.y == 0)
	{
		return false;
	}

	if (clientSize.x != myClientSize.x || clientSize.y != myClientSize.y)
	{
		myCommandList.ResetCommandList();
		if (!aGraphics.Resize(clientSize.x, clientSize.y))
		{
			throw std::runtime_error("Failed to resize rendering targets");
		}
		myClientSize = clientSize;
	}

	return true;
}

void GameApplication::RenderFrame(GraphicsEngine& aGraphics)
{
	WorldRenderer::Build(*myWorld, aGraphics, mySnapshot);

	// Render pass notification overlay
	if (myRenderPassNotificationWidget && myRenderPassNotificationRemainingSeconds > 0.0f)
	{
		const float opacity = myRenderPassNotificationRemainingSeconds >= RenderPassNotificationFadeDurationSeconds
			? 1.0f
			: myRenderPassNotificationRemainingSeconds / RenderPassNotificationFadeDurationSeconds;

		myRenderPassNotificationWidget->SetOpacity(opacity);

		mySnapshot.ScreenTextItems.push_back(myRenderPassNotificationWidget);
	}

	myCommandList.ResetCommandList();
	
	aGraphics.RenderSnapshot(myCommandList, mySnapshot);
	
	if (myCommandList.FinishCommandList())
	{
		aGraphics.ExecuteCommandList(myCommandList);
		aGraphics.Present();
	}
}

void GameApplication::ProcessPendingSceneLoad(Game& aGame)
{
	const SceneId requestedSceneId = *myPendingSceneId;
	// Consume the request so it is not loaded again next frame
	myPendingSceneId.reset();

	const GameSceneDefinition sceneDefinition = GetGameSceneDefinition(requestedSceneId);
	const std::string sceneName(sceneDefinition.DisplayName);
	
	std::unique_ptr<World> candidateWorld;
	try
	{
		// Build and configure a replacemet before changing the running world.
		AssetRegistry& assets = ServiceLocator::GetInstance().GetAssetRegistry();
		UnrealImportResult importResult = UnrealSceneImporter{}.ImportScene(myContentRoot / sceneDefinition.RelativeFile);
		if (!importResult)
		{
			std::ostringstream message;
			message << "Scene import failed:";
			for (const ImportDiagnostic& diagnostic : importResult.Diagnostics)
			{
				message << "\n - " << diagnostic.Context << ": " << diagnostic.Message;
			}
			throw std::runtime_error(message.str());
		}
		SceneData scene = std::move(*importResult.Data);
		LOG(LogGameFramework, Log, "Loaded scene '{}' from '{}' ({} actors).", sceneDefinition.DisplayName, sceneDefinition.RelativeFile.string(), scene.Actors.size());
		
		SceneFallbackAssets fallbacks;
		// TODO: Replace the procedural cube with the dedicated missing-mesh asset.
		fallbacks.MissingMesh = std::make_shared<MeshAsset>(PrimitiveMeshBuilder::CreateCube());
		// TODO: Replace the default material with the dedicated error texture/material.
		fallbacks.MissingMaterial = assets.GetAsset<MaterialAsset>("Shaders/_DefaultMaterial.mat");
		if (!fallbacks.MissingMaterial)
		{
			throw std::runtime_error("Could not load fallback material: " + assets.GetLastError());
		}

		candidateWorld = BuildWorldFromSceneData(scene, assets, myClientSize, fallbacks);
		aGame.ConfigureWorld(*candidateWorld);
	}
	catch (const std::bad_alloc&)
	{
		throw;
	}
	catch (const std::exception& error)
	{
		// Keep the current world if a later scene load fails.
		// An initial load must succeed because there is no current world.
		LOG(LogGameFramework, Error, "Scene '{}': {}", sceneName, error.what());
		if (!myInitialWorldStarted)
		{
			throw;
		}
		return;
	}

	// The candidate is ready; replace the running world.
	if (myWorld)
	{
		myWorld->Clear();
	}
	myWorld = std::move(candidateWorld);
	myCurrentSceneId = requestedSceneId;

	// Select a debug camera if the scene did not provide an active camera.
	myToggleDebugCameraRequested = false;
	myDebugCamera.Reset();
	if (!myWorld->GetActiveCamera())
	{
		myWorld->SetActiveCamera(myDebugCamera.Ensure(*myWorld, myClientSize));
	}

	// Start the new world
	myWorld->BeginPlay();
}

void GameApplication::RecenterMouseLook()
{
	if (myConfig.EnableMouseLook && GetForegroundWindow() == myMainWindowHandle &&
	    myInputHandler.IsKeyDown(int(EKeyCode::MOUSERBUTTON)))
	{
		myInputHandler.CenterMouse();
	}
}

void GameApplication::ShowRenderPassNotification()
{
	if (!myRenderPassNotificationWidget)
	{
		return;
	}
	myRenderPassNotificationWidget->SetText(std::string("Render Pass: ") + GraphicsEngine::Get().GetRenderPassName());
	myRenderPassNotificationWidget->SetOpacity(1.0f);
	myRenderPassNotificationRemainingSeconds = RenderPassNotificationDurationSeconds;
}

void GameApplication::Cleanup(Game& aGame, std::exception_ptr& aFailure)
{
	// Stop new scene requests before game shutdown can invoke callbacks.
	myAcceptSceneRequests = false;
	if (myGameInitializationStarted)
	{
		AttemptCleanup(aFailure, "Game shutdown", &Game::Shutdown, aGame);
	}

	// Each step runs even if an earlier step fails. Release users of a service
	// before destroying that service, then close the window last.
	AttemptCleanup(aFailure, "World cleanup", &GameApplication::ClearWorld, *this);
	AttemptCleanup(aFailure, "Application input cleanup", &GameApplication::RemoveApplicationInputListeners, *this);
	AttemptCleanup(aFailure, "Render reference cleanup", &GameApplication::ReleaseRenderReferences, *this);
	AttemptCleanup(aFailure, "Service cleanup", &GameApplication::KillServices, *this);
	DestroyWindowIfCreated();
}

void GameApplication::ClearWorld()
{
	if (myWorld)
	{
		myWorld->Clear();
		myWorld.reset();
	}
}

void GameApplication::RemoveApplicationInputListeners()
{
	if (auto* input = ServiceLocator::GetInstance().GetInputMapper())
	{
		for (unsigned id : myApplicationInputListenerIds)
		{
			input->RemoveEventListener(id);
		}
	}
	myApplicationInputListenerIds.clear();
	myInputHandler.ReleaseMouse();
}

void GameApplication::ReleaseRenderReferences()
{
	mySnapshot.Clear();
	myRenderPassNotificationWidget.reset();
	myRenderFont.reset();
	myRenderPassNotificationRemainingSeconds = 0.0f;
}

void GameApplication::KillServices()
{
	ServiceLocator::GetInstance().KillServices();
}

void GameApplication::DestroyWindowIfCreated() noexcept
{
	if (myMainWindowHandle)
	{
		DestroyWindow(myMainWindowHandle);
		myMainWindowHandle = nullptr;
	}
}
