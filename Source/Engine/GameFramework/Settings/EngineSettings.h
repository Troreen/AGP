#pragma once

#include "EnumGamepadCode.h"
#include "EnumKeyCode.h"
#include "EnumPointerCode.h"

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class WindowMode
{
	Windowed,
	Borderless
};

struct WindowResolution
{
	unsigned Width;
	unsigned Height;
};

struct ApplicationSettings
{
	std::string Title = "TM Game";
	std::string WindowClassName = "Turkish Motors Game";
	unsigned WindowedWidth = 1920;
	unsigned WindowedHeight = 1080;
	WindowMode Mode = WindowMode::Borderless;
	bool Resizable = false;
	bool EnableRenderDiagnostics = true;
	bool EnableMouseLook = true;
	std::filesystem::path ContentPath = "Content";
	std::filesystem::path CursorPath;
	std::string InitialScene = "Blockout";
};

struct SoundSettings
{
	float MasterVolume = 1.0f;
	float MusicVolume = 0.35f;
	float SfxVolume = 1.0f;
};

struct ActionBindings
{
	std::optional<EKeyCode> Key;
	std::optional<EPointerCode> Pointer;
	std::optional<EGamepadCode> Gamepad;
};

struct InputSettings
{
	std::map<std::string, ActionBindings> Actions;
};

class EngineSettings final
{
public:
	using ApplicationApplyCallback = std::function<ApplicationSettings(const ApplicationSettings&)>;
	using SoundApplyCallback = std::function<void(const SoundSettings&)>;
	using InputApplyCallback = std::function<void(const InputSettings&)>;
	using AvailableResolutionsCallback = std::function<std::vector<WindowResolution>()>;

	EngineSettings(std::filesystem::path anExecutableDirectory, std::filesystem::path aSettingsDirectory);
	void Load();

	const ApplicationSettings& GetApplicationSettings() const { return myCurrentApplication; }
	const SoundSettings& GetSoundSettings() const { return myCurrentSound; }
	const InputSettings& GetInputSettings() const { return myCurrentInput; }
	const ApplicationSettings& GetDefaultApplicationSettings() const { return myDefaultApplication; }
	const SoundSettings& GetDefaultSoundSettings() const { return myDefaultSound; }
	const InputSettings& GetDefaultInputSettings() const { return myDefaultInput; }
	std::filesystem::path GetContentRoot() const;
	std::vector<WindowResolution> GetAvailableWindowedResolutions() const;

	void SetApplicationApplyCallback(ApplicationApplyCallback aCallback) { myApplicationApply = std::move(aCallback); }
	void SetSoundApplyCallback(SoundApplyCallback aCallback) { mySoundApply = std::move(aCallback); }
	void SetInputApplyCallback(InputApplyCallback aCallback) { myInputApply = std::move(aCallback); }
	void SetAvailableResolutionsCallback(AvailableResolutionsCallback aCallback) { myAvailableResolutions = std::move(aCallback); }

	void UpdateApplicationSettings(const ApplicationSettings& applicationSettings);
	void UpdateSoundSettings(const SoundSettings& soundSettings);
	// The supplied action map replaces the saved current action map.
	void UpdateInputSettings(const InputSettings& inputSettings);
	void ResetApplicationSettings();
	void ResetSoundSettings();
	void ResetInputSettings();
	void ResetAllSettings();

private:
	std::filesystem::path myExecutableDirectory;
	std::filesystem::path mySettingsDirectory;
	ApplicationSettings myDefaultApplication;
	ApplicationSettings myCurrentApplication;
	SoundSettings myDefaultSound;
	SoundSettings myCurrentSound;
	InputSettings myDefaultInput;
	InputSettings myCurrentInput;
	ApplicationApplyCallback myApplicationApply;
	SoundApplyCallback mySoundApply;
	InputApplyCallback myInputApply;
	AvailableResolutionsCallback myAvailableResolutions;
};
