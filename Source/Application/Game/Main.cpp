#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <stdexcept>
#include "Game.h"
#include "GameScene.h"
#include "GameLog.h"
#include "GameFramework/Runtime/GameApplication.h"
#include "StringHelpers.h"

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
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
		const DWORD length = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
		if (!length || length == MAX_PATH)
		{
			throw std::runtime_error("Cannot locate the executable");
		}

		GameApplication::Config config;
		config.ContentRoot = std::filesystem::path(executablePath).parent_path() / "Content";
		config.EnableRenderDiagnostics = true;
		config.EnableMouseLook = true;

		Game game;
		GameScene scene;
		return GameApplication{}.Run(game, config, [&scene](const SceneType& aScene, SceneLoadContext& context)
		{
			return scene.Load(aScene, context);
		});
	}
	catch (const std::exception& error)
    {
		std::string message = error.what();
		if (!str::is_valid_utf8(message))
        {
	        message = str::acp_to_utf8(message);
        }
		GAMELOG(Error, "Game stopped: {}", message);
		return 1;
    }
}
