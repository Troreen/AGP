#pragma once
#include "GameFramework/Scenes/SceneData.h"
class IGame;

class GameApplication
{
public:
	struct Config
	{
		unsigned Width = 1920;
		unsigned Height = 1080;
		std::wstring Title = L"AGP Game";
		std::filesystem::path ContentRoot;
		bool ShowWindow = true;
		bool EnableRenderDiagnostics = false; // F6 selects a render pass; P logs statistics.
		bool EnableMouseLook = false;
	};

	// Owns the main loop until quit. The caller keeps game/source captures alive.
	int Run(IGame& game, const Config& config, SceneSource source = {});

private:
	class Impl;
};
