#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "InputHandler.h"
#include "Vector2.hpp"
#include "XInputHandler.h"

#include <exception>
#include <filesystem>
#include <GameFramework/Components/DebugCameraController.h>
#include <GameFramework/Settings/EngineSettings.h>
#include <GameFramework/Settings/InputSettingsApplier.h>
#include <GameFramework/Settings/WindowSettings.h>
#include <GraphicsEngine/GraphicsEngine.h>
#include <GraphicsEngine/GraphicsRenderSettings.h>
#include <GraphicsEngine/RHI/GraphicsCommandList.h>
#include <memory>
#include <string>
#include <vector>

class FontAsset;
class Game;
class TextWidget;
class World;

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

private:
	GraphicsEngine& InitializeWindowAndGraphics();
	void InitializeServices();
	void InitializeInputAndApplicationControls();
	void ApplySoundSettings(const SoundSettings& soundSettings);

	void RunSession(Game& aGame);
	void InitializeGameSession(Game& aGame, GraphicsEngine& aGraphics);
	void RunMainLoop(Game& aGame, GraphicsEngine& aGraphics);
	bool PrepareRenderTargetSize(GraphicsEngine& aGraphics);
	void RenderFrame(float aDeltaTime, GraphicsEngine& aGraphics);

	void ShowRenderPassNotification();
	
	void Cleanup(Game& aGame, std::exception_ptr& aFailure);
	void ClearScene();
	void RemoveApplicationInputListeners();
	void ReleaseRenderReferences();
	void KillServices();
	void DestroyWindowIfCreated() noexcept;

private:
	ApplicationSettings myApplicationSettings;
	WindowSettings myWindowSettings;
	InputSettingsApplier myInputSettingsApplier;
	DebugCameraService myDebugCamera;

	HWND myMainWindowHandle = nullptr;
	HCURSOR myCustomCursor = nullptr;
	std::wstring myWindowClassName;

	std::filesystem::path myContentRoot;
	CommonUtilities::Vector2u myClientSize{};

	CommonUtilities::InputHandler myInputHandler;
	CommonUtilities::XInputHandler myXInputHandler;
	std::vector<unsigned> myApplicationInputListenerIds;

	GraphicsCommandList myCommandList;
	GraphicsRenderSettings myRenderSettings;

	std::shared_ptr<TextWidget> myRenderPassNotificationWidget;
	std::shared_ptr<FontAsset> myRenderFont;

	float myRenderPassNotificationRemainingSeconds = 0.0f;

	bool myWindowClassRegistered = false;
	bool myGameInitializationStarted = false;
	bool myQuitRequested = false;
	bool myToggleDebugCameraRequested = false;
	bool myShowDebugWindow = false;
};
