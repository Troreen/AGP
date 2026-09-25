#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include "Game.h"
#include "GameApplication.h"
#include "GameLog.h"
#include "GameFramework/Settings/EngineSettings.h"
#include "GameFramework/ServiceLocator.h"
#include "StringHelpers.h"

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	bool applicationStarted = false;
#ifdef _DEBUG
	AllocConsole();
	FILE* output = nullptr;
	freopen_s(&output, "CONOUT$", "w", stdout);
	freopen_s(&output, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8);
#endif

	try
	{
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
		auto settingsService = std::make_unique<EngineSettings>(executableDirectory, settingsDirectory);
		settingsService->Load();
		ServiceLocator::GetInstance().SetEngineSettings(settingsService.release());

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
