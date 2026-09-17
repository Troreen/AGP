#pragma once
#include <filesystem>
#include <string>
class IGame;

namespace GameFrameworkIntegration
{
	struct ApplicationSetup;
}

// Engine-owned host. Each game supplies configuration and an IGame instance.
// Reusable runtime boundary: keep window handling, scheduling and rendering here,
// not in individual games. A repository currently contains one game; replacing that
// game should change its entry point/content/IGame, not this host.
// New game developers generally only need Config and Run.
class GameApplication
{
public:
	struct Config
	{
		unsigned Width = 1920;
		unsigned Height = 1080;
		std::wstring Title = L"AGP Game";
		// Runtime content directory containing Shaders and the game assets. Paths are
		// configured by the game; the framework does not know the source-tree layout.
		std::filesystem::path ContentRoot;
		// Fixed simulation step in seconds, required to be positive and finite.
		// Variable Update still uses elapsed gameplay-frame time.
		float FixedDeltaTime = 1.0f / 60.0f;
		// A debugging option, not a different gameplay API. Both modes use the same
		// callback sequence. AGP_DISABLE_THREADED_UPDATE also forces synchronous mode.
		bool ThreadedUpdate = true;
		// Hidden windows support automated host tests with the real graphics path.
		bool ShowWindow = true;
		// Optional engine diagnostics: F6 cycles render passes and P prints statistics.
		bool EnableRenderDiagnostics = false;
		bool EnableMouseLook = false; // Hold RMB for relative mouse input.
	};

	// Blocks until exit; joins gameplay work before returning or throwing.
	// The caller owns game and must keep it alive for this blocking call. Exceptions
	// return to the caller after worker cleanup. This is a single-session host; live
	// scene replacement is supported; repeated renderer initialization is not promised.
	int Run(IGame& game, const Config& config);
	int Run(IGame& game, const Config& config, GameFrameworkIntegration::ApplicationSetup setup);

private:
	class Impl;
};
