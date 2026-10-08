#pragma once
#include "GameApplication.h"
#include "GameWindowMessages.h"

#include "Game.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/FontAsset.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/Coroutine/Scheduler.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/Animation/AnimationManager.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Settings/EngineSettings.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GameFramework/Rendering/WorldRenderer.h"
#include "GameFramework/Scenes/WorldFromSceneData.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GameFramework/World/World.h"
#include "GraphicsEngine/TextWidget.h"
#include "InputMapper.h"
#include "Maths.hpp"
#include "PrimitiveMeshBuilder.h"
#include "StringHelpers.h"
#include "Timer.h"
#include <GraphicsEngine/Drawers/SpriteDrawer.h>

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

GameApplication::GameApplication() = default;

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
		// Show the error before cleanup closes the window and services.
		try
		{
			std::rethrow_exception(failure);
		}
		catch (const std::exception& error)
		{
			MessageBoxA(myMainWindowHandle, error.what(), "AGP Game error", MB_OK | MB_ICONERROR);
		}
		catch (...)
		{
			MessageBoxA(myMainWindowHandle, "Unknown application error", "AGP Game error", MB_OK | MB_ICONERROR);
		}
	}

	// Clean up after normal exit or failure, preserving the first exception.
	Cleanup(aGame, failure);

	// Main logs the failure after cleanup has been attempted.
	if (failure)
	{
		std::rethrow_exception(failure);
	}
	return 0;
}

void GameApplication::RunSession(Game& aGame)
{
	EngineSettings& settings = ServiceLocator::GetInstance().GetEngineSettings();
	settings.SetAvailableResolutionsCallback([this]()
	{
		return WindowSettings::GetAvailableResolutions(WindowSettings::GetMonitor(myMainWindowHandle));
	});

	// Apply the loaded window settings to the starting monitor.
	myApplicationSettings = WindowSettings::CapResolution(settings.GetApplicationSettings(), WindowSettings::GetMonitor(nullptr));
	if (myApplicationSettings.WindowedWidth != settings.GetApplicationSettings().WindowedWidth ||
		myApplicationSettings.WindowedHeight != settings.GetApplicationSettings().WindowedHeight)
	{
		settings.UpdateApplicationSettings(myApplicationSettings);
	}
	myContentRoot = settings.GetContentRoot();

	GraphicsEngine& graphics = InitializeWindowAndGraphics();
	InitializeServices();
	InitializeInputAndApplicationControls();

	// Apply later settings changes through the initialized window, audio, and input services.
	settings.SetApplicationApplyCallback([this](const ApplicationSettings& requested)
	{
		ApplicationSettings effective = myWindowSettings.Apply(myMainWindowHandle, requested, myApplicationSettings.Mode);
		myInputHandler.SetMouseDeltaEnabled(effective.EnableMouseLook);
		myApplicationSettings = effective;
		return effective;
	});
	settings.SetSoundApplyCallback([this](const SoundSettings& requested)
	{
		ApplySoundSettings(requested);
	});
	settings.SetInputApplyCallback([this](const InputSettings& requested)
	{
		myInputSettingsApplier.Apply(ServiceLocator::GetInstance().GetInputMapper(), requested);
	});

	myGameInitializationStarted = true;
	InitializeGameSession(aGame, graphics);
	RunMainLoop(aGame, graphics);
}

bool GameApplication::RequestSceneLoad(const std::filesystem::path& aScene)
{
	if (!myAcceptSceneRequests)
	{
		return false;
	}
	// pendingScene is consumed in the next frame by the update loop
	myPendingScene = aScene;
	return true;
}

// TODO: this probably should be moved to a sceneloader class or similar, since it is not really a GameApplication concern.
bool GameApplication::ReloadCurrentScene()
{
	return myCurrentScene && RequestSceneLoad(*myCurrentScene);
}

GraphicsEngine& GameApplication::InitializeWindowAndGraphics()
{
	// Window class and creation
	myWindowClassName = str::utf8_to_wide(myApplicationSettings.WindowClassName);
	const wchar_t* className = myWindowClassName.c_str();
	WNDCLASSW windowClass = {};
	windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = GameWindowMessages::WindowProc;

	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	if (!myApplicationSettings.CursorPath.empty())
	{
		const std::filesystem::path cursorFile = myApplicationSettings.CursorPath.is_absolute()
			? myApplicationSettings.CursorPath : myContentRoot / myApplicationSettings.CursorPath;
		if (std::filesystem::exists(cursorFile))
		{
			HCURSOR sourceCursor = static_cast<HCURSOR>(LoadImageW(nullptr, cursorFile.c_str(), IMAGE_CURSOR,
				64, 64, LR_LOADFROMFILE));
			if (sourceCursor)
			{
				ICONINFO info{};
				if (GetIconInfo(sourceCursor, &info))
				{
					info.fIcon = FALSE;
					info.xHotspot = 32;
					info.yHotspot = 32;
					myCustomCursor = CreateIconIndirect(&info);
					DeleteObject(info.hbmColor);
					DeleteObject(info.hbmMask);
				}
				DestroyCursor(sourceCursor);
				if (myCustomCursor)
				{
					windowClass.hCursor = myCustomCursor;
				}
			}
		}
	}
	windowClass.hInstance = GetModuleHandleW(nullptr);
	windowClass.lpszClassName = className;

	if (RegisterClassW(&windowClass))
	{
		myWindowClassRegistered = true;
	}
	else if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
	{
		throw std::runtime_error("Could not register game window");
	}

	const bool borderless = myApplicationSettings.Mode == WindowMode::Borderless;
	const DWORD style = WindowSettings::GetWindowStyle(myApplicationSettings.Mode);
	RECT windowBounds{0, 0, static_cast<LONG>(myApplicationSettings.WindowedWidth), static_cast<LONG>(myApplicationSettings.WindowedHeight)};
	int windowX = CW_USEDEFAULT;
	int windowY = CW_USEDEFAULT;
	if (borderless)
	{
		windowBounds = WindowSettings::GetMonitor(nullptr).Bounds;
		windowX = windowBounds.left;
		windowY = windowBounds.top;
	}
	else if (!AdjustWindowRectEx(&windowBounds, style, FALSE, 0))
	{
		throw std::runtime_error("Could not calculate the window size");
	}
	const std::wstring windowTitle = str::utf8_to_wide(myApplicationSettings.Title);
	myMainWindowHandle = CreateWindowW(className, windowTitle.c_str(), style, windowX, windowY,
		windowBounds.right - windowBounds.left, windowBounds.bottom - windowBounds.top,
		nullptr, nullptr, windowClass.hInstance, &myInputHandler);

	if (!myMainWindowHandle)
	{
		throw std::runtime_error("Could not create game window");
	}

	myApplicationSettings = myWindowSettings.Apply(myMainWindowHandle, myApplicationSettings, myApplicationSettings.Mode);

	// Initialize graphics engine
	GraphicsEngine& graphics = GraphicsEngine::Get();
	if (!graphics.Initialize(myMainWindowHandle, myContentRoot / myApplicationSettings.ShaderFolder) || 
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

	// TODO: does this belong here?
	if (myApplicationSettings.EnableRenderDiagnostics)
	{
		AssetRegistry& assets = services.GetAssetRegistry();
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

	// The buses must exist before applying saved volumes.
	EngineSettings& settings = services.GetEngineSettings();
	ApplySoundSettings(settings.GetSoundSettings());
}

void GameApplication::ApplySoundSettings(const SoundSettings& soundSettings)
{
	AudioManager& audio = ServiceLocator::GetInstance().GetAudioManager();
	audio.SetMasterVolume(soundSettings.MasterVolume);
	audio.SetBusVolume(BusID::eMusic, soundSettings.MusicVolume);
	audio.SetBusVolume(BusID::eSFX, soundSettings.SfxVolume);
}

void GameApplication::InitializeInputAndApplicationControls()
{
	myInputHandler.SetWindowHandle(myMainWindowHandle);
	myInputHandler.SetAutoMouseCapture(false);
	myInputHandler.SetMouseDeltaEnabled(myApplicationSettings.EnableMouseLook);

	CU::InputMapper& input = ServiceLocator::GetInstance().GetInputMapper();
	input.Init(&myInputHandler, &myXInputHandler);

	// Install saved bindings before the game adds listeners to their action names.
	EngineSettings& settings = ServiceLocator::GetInstance().GetEngineSettings();
	myInputSettingsApplier.Apply(input, settings.GetInputSettings());

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
	if (myApplicationSettings.EnableRenderDiagnostics)
	{
		myApplicationInputListenerIds.push_back(input.AddEventListener("PreviousRenderPass", [this](const CommonUtilities::InputEvent& anEvent)
		{
			if (anEvent.inputData.isPressed)
			{
				const uint32_t currentPass = static_cast<uint32_t>(myRenderSettings.SelectedRenderPass);
				constexpr uint32_t maxPass = static_cast<uint32_t>(RenderPass::Count);
				myRenderSettings.SelectedRenderPass = static_cast<RenderPass>(currentPass == 0 ? maxPass - 1 : currentPass - 1);
				ShowRenderPassNotification();
			}
		}));
		myApplicationInputListenerIds.push_back(input.AddEventListener("NextRenderPass", [this](const CommonUtilities::InputEvent& anEvent)
		{
			if (anEvent.inputData.isPressed)
			{
				const uint32_t currentPass = static_cast<uint32_t>(myRenderSettings.SelectedRenderPass);
				constexpr uint32_t maxPass = static_cast<uint32_t>(RenderPass::Count);
				myRenderSettings.SelectedRenderPass = static_cast<RenderPass>((currentPass + 1) % maxPass);
				ShowRenderPassNotification();
			}
		}));
	}
}

void GameApplication::InitializeGameSession(Game& aGame, GraphicsEngine& aGraphics)
{
	aGame.Initialize(*this);

	// Start the scene named in the settings unless the game requested one.
	if (!myPendingScene)
	{
		RequestSceneLoad(myApplicationSettings.InitialScene);
	}

	myInitialWorldStarted = true;
	
	ShowWindow(myMainWindowHandle, SW_SHOW);
	SetForegroundWindow(myMainWindowHandle);
	aGraphics.Present();

	ProcessPendingSceneLoad(aGame);
}

void GameApplication::RunMainLoop(Game& aGame, GraphicsEngine& aGraphics)
{
	// Initialise timer
	CommonUtilities::Timer timer;
	timer.Update();

	// Main loop
	while (!myQuitRequested)
	{
		GameWindowMessages::Pump(myQuitRequested);
		if (myQuitRequested)
		{
			break;
		}
		if (myWindowSettings.MonitorChanged(myMainWindowHandle))
		{
			ServiceLocator::GetInstance().GetEngineSettings().UpdateApplicationSettings(myApplicationSettings);
		}

		// Process any pending scene load requests before ticking the timer, so that the first frame of a new scene is not delayed by a full frame time.
		if (myPendingScene)
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
		ServiceLocator::GetInstance().GetInputMapper().Update();
		
		if (myQuitRequested)
		{
			break;
		}

		// Handle debug camera toggle
		if (std::exchange(myToggleDebugCameraRequested, false))
		{
			myDebugCamera.Toggle(*myWorld, myClientSize);
		}

		// Update game and world
		aGame.Update(*myWorld, delta);
		myWorld->Update(delta);
		ServiceLocator::GetInstance().GetAudioManager().Update(delta);

		ServiceLocator::GetInstance().GetScheduler().IncreaseTimers(delta);
		ServiceLocator::GetInstance().GetScheduler().Update();

		RenderFrame(aGraphics);
	}
}

bool GameApplication::PrepareRenderTargetSize(GraphicsEngine& aGraphics)
{
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

		if (myWorld && !myWorld->SetCameraResolution(clientSize))
		{
			throw std::runtime_error("Failed to resize the cameras");
		}
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

	std::vector<SpriteDrawCall> spriteDrawCalls = SpriteDrawer::Get().GetDrawCalls();

	std::sort(spriteDrawCalls.begin(), spriteDrawCalls.end(), [](const SpriteDrawCall& A, const SpriteDrawCall& B)
		{
			return A.SpriteInstanceData->Depth < B.SpriteInstanceData->Depth;
		});

	mySnapshot.ScreenSpriteItems = spriteDrawCalls;

	aGraphics.RenderSnapshot(myCommandList, mySnapshot, myRenderSettings);
	
	if (myCommandList.FinishCommandList())
	{
		aGraphics.ExecuteCommandList(myCommandList);
		aGraphics.Present();
	}
}

// TODO: this should be moved to a sceneloader class or similar aswell
void GameApplication::ProcessPendingSceneLoad(Game& aGame)
{
	const std::filesystem::path requestedScene = *myPendingScene;
	// Consume the request so it is not loaded again next frame
	myPendingScene.reset();

	const std::string sceneName = requestedScene.stem().string();
	std::filesystem::path scenePath = myContentRoot / myApplicationSettings.SceneFolder / requestedScene;
	
	std::unique_ptr<World> candidateWorld;
	try
	{
		if (!std::filesystem::exists(scenePath))
		{
			throw std::runtime_error(
				std::format("Scene '{}' does not exist within the scene folder '{}' in the content folder!",
					requestedScene.string(),
					myApplicationSettings.SceneFolder));
		}

		// Build and configure a replacemet before changing the running world.
		AssetRegistry& assets = ServiceLocator::GetInstance().GetAssetRegistry();
		UnrealImportResult importResult = UnrealSceneImporter{}.ImportScene(scenePath);
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
		LOG(LogGameFramework, Log, "Loaded scene '{}' from '{}' ({} actors).", sceneName, requestedScene.string(), scene.Actors.size());
		
		SceneFallbackAssets fallbacks;
		// TODO: Replace the procedural cube with the dedicated missing-mesh asset.
		fallbacks.MissingMesh = std::make_shared<MeshAsset>(PrimitiveMeshBuilder::CreateCube());
		fallbacks.MissingShader = myContentRoot / myApplicationSettings.ShaderFolder / "_DefaultMaterial.hlsli";

		MaterialDescription matDesc;
		matDesc.Name = "_Default Material_";
		matDesc.Domain = MaterialDomain::Surface;
		matDesc.ShadingModel = ShadingModel::Lit;
		matDesc.BlendMode = BlendMode::Opaque;
		matDesc.MaterialShaderCode = fallbacks.MissingShader;

		std::shared_ptr<Material> material = std::make_shared<Material>();
		if (!GraphicsEngine::Get().CreateMaterial(matDesc, *material))
		{
			LOG(LogGameFramework, Warning, "Could not create default material!");
		}
		else
		{
			fallbacks.MissingMaterial = std::make_shared<MaterialAsset>(material);
		}

		WorldFromSceneConverter converter;
		converter.Initialize(ServiceLocator::GetInstance().GetEngineSettings().GetSettingsDirectory() / "MaterialNameConversions.json");
		candidateWorld = converter.BuildWorldFromSceneData(scene, assets, myContentRoot / myApplicationSettings.ShaderFolder, myClientSize, fallbacks);
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
	myCurrentScene = requestedScene;

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

void GameApplication::ShowRenderPassNotification()
{
	if (!myRenderPassNotificationWidget)
	{
		return;
	}
	myRenderPassNotificationWidget->SetText(std::string("Render Pass: ") + GraphicsEngine::RenderSettings::GetRenderPassName(myRenderSettings.SelectedRenderPass));
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
	CommonUtilities::InputMapper& input = ServiceLocator::GetInstance().GetInputMapper();
	for (unsigned id : myApplicationInputListenerIds)
	{
		input.RemoveEventListener(id);
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

	if (myWindowClassRegistered)
	{
		UnregisterClassW(myWindowClassName.c_str(), GetModuleHandleW(nullptr));
		myWindowClassRegistered = false;
	}

	if (myCustomCursor)
	{
		DestroyCursor(myCustomCursor);
		myCustomCursor = nullptr;
	}
}
