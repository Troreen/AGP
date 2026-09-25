#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "GameFramework/Components/DebugCameraController.h"
#include "GameFramework/Settings/EngineSettings.h"
#include "GameFramework/Settings/InputSettingsApplier.h"
#include "GameFramework/Settings/WindowSettings.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/RHI/GraphicsCommandList.h"
#include "InputHandler.h"
#include "Vector2.hpp"
#include "XInputHandler.h"

#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class FontAsset;
class Game;
class TextWidget;
class World;

enum class SceneId
{
	Blockout,
	Chests,
	ChestMaterials
};

// Owns one game run: platform state, the main loop, the live World and scene requests.
class GameApplication final
{
public:
	GameApplication();
	~GameApplication();
	GameApplication(const GameApplication&) = delete;
	GameApplication& operator=(const GameApplication&) = delete;
	GameApplication(GameApplication&&) = delete;
	GameApplication& operator=(GameApplication&&) = delete;

	int Run(Game& aGame);

	// Requests are last-write-wins and are processed only at safe frame boundaries.
	bool RequestSceneLoad(SceneId aSceneId);
	bool ReloadCurrentScene();

private:
	GraphicsEngine& InitializeWindowAndGraphics();
	void InitializeServices();
	void InitializeInputAndApplicationControls();
	void ApplySoundSettings(const SoundSettings& soundSettings);

	void RunSession(Game& aGame);
	void InitializeGameSession(Game& aGame);
	void RunMainLoop(Game& aGame, GraphicsEngine& aGraphics);
	bool PrepareRenderTargetSize(GraphicsEngine& aGraphics);
	void RenderFrame(GraphicsEngine& aGraphics);
	void ProcessPendingSceneLoad(Game& aGame);

	void ShowRenderPassNotification();
	
	void Cleanup(Game& aGame, std::exception_ptr& aFailure);
	void ClearWorld();
	void RemoveApplicationInputListeners();
	void ReleaseRenderReferences();
	void KillServices();
	void DestroyWindowIfCreated() noexcept;

	ApplicationSettings myApplicationSettings;
	std::filesystem::path myContentRoot;
	std::unique_ptr<World> myWorld;
	std::optional<SceneId> myPendingSceneId;
	std::optional<SceneId> myCurrentSceneId;
	CommonUtilities::Vector2u myClientSize{};
	bool myQuitRequested = false;
	bool myAcceptSceneRequests = true;
	bool myGameInitializationStarted = false;
	bool myInitialWorldStarted = false;

	WindowSettings myWindowSettings;
	HWND myMainWindowHandle = nullptr;
	std::wstring myWindowClassName;
	HCURSOR myCustomCursor = nullptr;
	bool myWindowClassRegistered = false;
	InputSettingsApplier myInputSettingsApplier;
	CommonUtilities::InputHandler myInputHandler;
	CommonUtilities::XInputHandler myXInputHandler;
	std::vector<unsigned> myApplicationInputListenerIds;

	GraphicsCommandList myCommandList;
	GraphicsEngine::RenderSceneSnapshot mySnapshot;
	GraphicsEngine::RenderSettings myRenderSettings;
	std::shared_ptr<TextWidget> myRenderPassNotificationWidget;
	std::shared_ptr<FontAsset> myRenderFont;
	float myRenderPassNotificationRemainingSeconds = 0.0f;

	bool myToggleDebugCameraRequested = false;
	DebugCameraService myDebugCamera;
};
