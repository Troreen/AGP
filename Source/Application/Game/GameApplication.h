#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "GameFramework/Components/DebugCameraController.h"
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
	struct Config
	{
		unsigned Width = 1920;
		unsigned Height = 1080;
		std::wstring Title = L"AGP Game";
		std::filesystem::path ContentRoot;
		bool EnableRenderDiagnostics = false;
		bool EnableMouseLook = false;
	};

	explicit GameApplication(Config aConfig);
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

	void RunSession(Game& aGame);
	void StartGameSession(Game& aGame);
	void RunMainLoop(Game& aGame, GraphicsEngine& aGraphics);
	void PumpWindowMessages();
	bool PrepareRenderTargetSize(GraphicsEngine& aGraphics);
	void RenderFrame(GraphicsEngine& aGraphics);
	void ProcessPendingSceneLoad(Game& aGame);

	void RecenterMouseLook();
	void ShowRenderPassNotification();
	
	void Cleanup(Game& aGame, std::exception_ptr& aFailure);
	void ClearWorld();
	void RemoveApplicationInputListeners();
	void ReleaseRenderReferences();
	void KillServices();
	void DestroyWindowIfCreated() noexcept;

	Config myConfig;
	std::filesystem::path myContentRoot;
	std::unique_ptr<World> myWorld;
	std::optional<SceneId> myPendingSceneId;
	std::optional<SceneId> myCurrentSceneId;
	CommonUtilities::Vector2u myClientSize{};
	bool myQuitRequested = false;
	bool myAcceptSceneRequests = true;
	bool myGameInitializationStarted = false;
	bool myInitialWorldStarted = false;

	HWND myMainWindowHandle = nullptr;
	CommonUtilities::InputHandler myInputHandler;
	CommonUtilities::XInputHandler myXInputHandler;
	std::vector<unsigned> myApplicationInputListenerIds;

	GraphicsCommandList myCommandList;
	GraphicsEngine::RenderSceneSnapshot mySnapshot;
	std::shared_ptr<TextWidget> myRenderPassNotificationWidget;
	std::shared_ptr<FontAsset> myRenderFont;
	float myRenderPassNotificationRemainingSeconds = 0.0f;

	bool myToggleDebugCameraRequested = false;
	DebugCameraService myDebugCamera;
};
