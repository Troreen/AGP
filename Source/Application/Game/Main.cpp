#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>
#include <filesystem>
#include <stdexcept>

#include "Game.h"
#include "GameApplication.h"
#include "GameFramework/Animation/AnimationManager.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/Settings/EngineSettings.h"
#include "GameLog.h"

#include <StringHelpers.h>

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	bool applicationStarted = false;
#ifndef _RETAIL
	AllocConsole();
	FILE* output = nullptr;
	freopen_s(&output, "CONOUT$", "w", stdout);
	freopen_s(&output, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8);
#endif

	try
	{
		ServiceLocator::GetInstance().Initialize();

		wchar_t executablePath[MAX_PATH] = {};
		const DWORD executablePathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
		if (executablePathLength == 0 || executablePathLength == MAX_PATH)
		{
			throw std::runtime_error("Cannot locate the executable");
		}

		// Read settings before creating the window or engine services.
		const std::filesystem::path executableDirectory = std::filesystem::path(executablePath).parent_path();
		const std::filesystem::path settingsDirectory = executableDirectory.parent_path() / "Settings";
		GAMELOG(Log, "Loading settings from {}", settingsDirectory.string());

		EngineSettings& settings = ServiceLocator::GetInstance().GetEngineSettings();
		settings.Load(executableDirectory, settingsDirectory);

		const std::filesystem::path contentRoot = settings.GetContentRoot();

		AssetRegistry& assets = ServiceLocator::GetInstance().GetAssetRegistry();
		assets.Initialize(contentRoot);
		if (!assets.IsInitialized())
		{
			throw std::runtime_error(assets.GetLastError());
		}
		ServiceLocator::GetInstance().GetAnimationManager().Initialize(contentRoot);
		ServiceLocator::GetInstance().GetAudioManager().Init();

		Game game;
		GameApplication application;
		applicationStarted = true;
		return application.Run(game);
	}
	catch (const std::exception& error)
	{
		std::string failureMessage = error.what();
		if (!str::is_valid_utf8(failureMessage))
		{
			failureMessage = str::acp_to_utf8(failureMessage);
		}

		GAMELOG(Error, "Game stopped: {}", failureMessage);

		// Run reports its own errors before cleanup; startup errors are shown here.
		if (!applicationStarted)
		{
			MessageBoxA(nullptr, failureMessage.c_str(), "AGP Game error", MB_OK | MB_ICONERROR);
		}

		return 1;
	}
}
