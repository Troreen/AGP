#pragma once
#include "GameFramework/Scenes/SceneData.h"
#include <algorithm>
#include <memory>
class IGame;
class Font;
class FontHandle;

class RenderPassNotificationTimer
{
public:
	void Restart() { myRemaining = Duration; }
	void Update(float delta) { myRemaining = (std::max)(0.0f, myRemaining - (std::max)(0.0f, delta)); }
	bool IsVisible() const { return myRemaining > 0.0f; }
	float GetOpacity() const { return myRemaining >= FadeDuration ? 1.0f : myRemaining / FadeDuration; }
	float GetRemaining() const { return myRemaining; }

	static constexpr float Duration = 2.0f;
	static constexpr float FadeDuration = 0.5f;

private:
	float myRemaining = 0.0f;
};

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
		bool EnableRenderDiagnostics = false; // F5/F6 select render passes; P logs statistics.
		bool EnableMouseLook = false;
	};

	// Owns the main loop until quit. The caller keeps game/source captures alive.
	int Run(IGame& game, const Config& config, SceneSource source = {});

private:
	static std::shared_ptr<Font> GetFontResource(const FontHandle& asset);
	class Impl;
};
