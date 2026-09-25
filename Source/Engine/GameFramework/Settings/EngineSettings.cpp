#include "EngineSettings.h"
#include <nlohmann/json.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
	using Json = nlohmann::ordered_json;

	struct NamedCode
	{
		const char* Name;
		int Code;
	};

	constexpr NamedCode KeyCodes[] =
	{
		{"MOUSELBUTTON", int(EKeyCode::MOUSELBUTTON)},
		{"MOUSERBUTTON", int(EKeyCode::MOUSERBUTTON)},
		{"CANCEL", int(EKeyCode::CANCEL)},
		{"MBUTTON", int(EKeyCode::MBUTTON)},
		{"MOUSEXBUTTON1", int(EKeyCode::MOUSEXBUTTON1)},
		{"MOUSEXBUTTON2", int(EKeyCode::MOUSEXBUTTON2)},
		{"BACK", int(EKeyCode::BACK)},
		{"TAB", int(EKeyCode::TAB)},
		{"CLEAR", int(EKeyCode::CLEAR)},
		{"RETURN", int(EKeyCode::RETURN)},
		{"SHIFT", int(EKeyCode::SHIFT)},
		{"CONTROL", int(EKeyCode::CONTROL)},
		{"MENU", int(EKeyCode::MENU)},
		{"PAUSE", int(EKeyCode::PAUSE)},
		{"CAPITAL", int(EKeyCode::CAPITAL)},
		{"KANA", int(EKeyCode::KANA)},
		{"HANGEUL", int(EKeyCode::HANGEUL)},
		{"HANGUL", int(EKeyCode::HANGUL)},
		{"JUNJA", int(EKeyCode::JUNJA)},
		{"FINAL", int(EKeyCode::FINAL)},
		{"HANJA", int(EKeyCode::HANJA)},
		{"KANJI", int(EKeyCode::KANJI)},
		{"ESCAPE", int(EKeyCode::ESCAPE)},
		{"CONVERT", int(EKeyCode::CONVERT)},
		{"NONCONVERT", int(EKeyCode::NONCONVERT)},
		{"ACCEPT", int(EKeyCode::ACCEPT)},
		{"MODECHANGE", int(EKeyCode::MODECHANGE)},
		{"SPACE", int(EKeyCode::SPACE)},
		{"PRIOR", int(EKeyCode::PRIOR)},
		{"NEXT", int(EKeyCode::NEXT)},
		{"END", int(EKeyCode::END)},
		{"HOME", int(EKeyCode::HOME)},
		{"LEFT", int(EKeyCode::LEFT)},
		{"UP", int(EKeyCode::UP)},
		{"RIGHT", int(EKeyCode::RIGHT)},
		{"DOWN", int(EKeyCode::DOWN)},
		{"SELECT", int(EKeyCode::SELECT)},
		{"PRINT", int(EKeyCode::PRINT)},
		{"EXECUTE", int(EKeyCode::EXECUTE)},
		{"SNAPSHOT", int(EKeyCode::SNAPSHOT)},
		{"INSERT", int(EKeyCode::INSERT)},
		{"DELETE_BUTTON", int(EKeyCode::DELETE_BUTTON)},
		{"HELP", int(EKeyCode::HELP)},
		{"A", int(EKeyCode::A)},
		{"B", int(EKeyCode::B)},
		{"C", int(EKeyCode::C)},
		{"D", int(EKeyCode::D)},
		{"E", int(EKeyCode::E)},
		{"F", int(EKeyCode::F)},
		{"G", int(EKeyCode::G)},
		{"H", int(EKeyCode::H)},
		{"I", int(EKeyCode::I)},
		{"J", int(EKeyCode::J)},
		{"K", int(EKeyCode::K)},
		{"L", int(EKeyCode::L)},
		{"M", int(EKeyCode::M)},
		{"N", int(EKeyCode::N)},
		{"O", int(EKeyCode::O)},
		{"P", int(EKeyCode::P)},
		{"Q", int(EKeyCode::Q)},
		{"R", int(EKeyCode::R)},
		{"S", int(EKeyCode::S)},
		{"T", int(EKeyCode::T)},
		{"U", int(EKeyCode::U)},
		{"V", int(EKeyCode::V)},
		{"W", int(EKeyCode::W)},
		{"X", int(EKeyCode::X)},
		{"Y", int(EKeyCode::Y)},
		{"Z", int(EKeyCode::Z)},
		{"LWIN", int(EKeyCode::LWIN)},
		{"RWIN", int(EKeyCode::RWIN)},
		{"APPS", int(EKeyCode::APPS)},
		{"SLEEP", int(EKeyCode::SLEEP)},
		{"NUMPAD0", int(EKeyCode::NUMPAD0)},
		{"NUMPAD1", int(EKeyCode::NUMPAD1)},
		{"NUMPAD2", int(EKeyCode::NUMPAD2)},
		{"NUMPAD3", int(EKeyCode::NUMPAD3)},
		{"NUMPAD4", int(EKeyCode::NUMPAD4)},
		{"NUMPAD5", int(EKeyCode::NUMPAD5)},
		{"NUMPAD6", int(EKeyCode::NUMPAD6)},
		{"NUMPAD7", int(EKeyCode::NUMPAD7)},
		{"NUMPAD8", int(EKeyCode::NUMPAD8)},
		{"NUMPAD9", int(EKeyCode::NUMPAD9)},
		{"MULTIPLY", int(EKeyCode::MULTIPLY)},
		{"ADD", int(EKeyCode::ADD)},
		{"SEPARATOR", int(EKeyCode::SEPARATOR)},
		{"SUBTRACT", int(EKeyCode::SUBTRACT)},
		{"DECIMAL", int(EKeyCode::DECIMAL)},
		{"DIVIDE", int(EKeyCode::DIVIDE)},
		{"F1", int(EKeyCode::F1)},
		{"F2", int(EKeyCode::F2)},
		{"F3", int(EKeyCode::F3)},
		{"F4", int(EKeyCode::F4)},
		{"F5", int(EKeyCode::F5)},
		{"F6", int(EKeyCode::F6)},
		{"F7", int(EKeyCode::F7)},
		{"F8", int(EKeyCode::F8)},
		{"F9", int(EKeyCode::F9)},
		{"F10", int(EKeyCode::F10)},
		{"F11", int(EKeyCode::F11)},
		{"F12", int(EKeyCode::F12)},
		{"F13", int(EKeyCode::F13)},
		{"F14", int(EKeyCode::F14)},
		{"F15", int(EKeyCode::F15)},
		{"F16", int(EKeyCode::F16)},
		{"F17", int(EKeyCode::F17)},
		{"F18", int(EKeyCode::F18)},
		{"F19", int(EKeyCode::F19)},
		{"F20", int(EKeyCode::F20)},
		{"F21", int(EKeyCode::F21)},
		{"F22", int(EKeyCode::F22)},
		{"F23", int(EKeyCode::F23)},
		{"F24", int(EKeyCode::F24)},
		{"NUMLOCK", int(EKeyCode::NUMLOCK)},
		{"SCROLL", int(EKeyCode::SCROLL)},
		{"OEM_NEC_EQUAL", int(EKeyCode::OEM_NEC_EQUAL)},
		{"OEM_FJ_JISHO", int(EKeyCode::OEM_FJ_JISHO)},
		{"OEM_FJ_MASSHOU", int(EKeyCode::OEM_FJ_MASSHOU)},
		{"OEM_FJ_TOUROKU", int(EKeyCode::OEM_FJ_TOUROKU)},
		{"OEM_FJ_LOYA", int(EKeyCode::OEM_FJ_LOYA)},
		{"OEM_FJ_ROYA", int(EKeyCode::OEM_FJ_ROYA)},
		{"LSHIFT", int(EKeyCode::LSHIFT)},
		{"RSHIFT", int(EKeyCode::RSHIFT)},
		{"LCONTROL", int(EKeyCode::LCONTROL)},
		{"RCONTROL", int(EKeyCode::RCONTROL)},
		{"LMENU", int(EKeyCode::LMENU)},
		{"RMENU", int(EKeyCode::RMENU)},
		{"BROWSER_BACK", int(EKeyCode::BROWSER_BACK)},
		{"BROWSER_FORWARD", int(EKeyCode::BROWSER_FORWARD)},
		{"BROWSER_REFRESH", int(EKeyCode::BROWSER_REFRESH)},
		{"BROWSER_STOP", int(EKeyCode::BROWSER_STOP)},
		{"BROWSER_SEARCH", int(EKeyCode::BROWSER_SEARCH)},
		{"BROWSER_FAVORITES", int(EKeyCode::BROWSER_FAVORITES)},
		{"BROWSER_HOME", int(EKeyCode::BROWSER_HOME)},
		{"VOLUME_MUTE", int(EKeyCode::VOLUME_MUTE)},
		{"VOLUME_DOWN", int(EKeyCode::VOLUME_DOWN)},
		{"VOLUME_UP", int(EKeyCode::VOLUME_UP)},
		{"MEDIA_NEXT_TRACK", int(EKeyCode::MEDIA_NEXT_TRACK)},
		{"MEDIA_PREV_TRACK", int(EKeyCode::MEDIA_PREV_TRACK)},
		{"MEDIA_STOP", int(EKeyCode::MEDIA_STOP)},
		{"MEDIA_PLAY_PAUSE", int(EKeyCode::MEDIA_PLAY_PAUSE)},
		{"LAUNCH_MAIL", int(EKeyCode::LAUNCH_MAIL)},
		{"LAUNCH_MEDIA_SELECT", int(EKeyCode::LAUNCH_MEDIA_SELECT)},
		{"LAUNCH_APP1", int(EKeyCode::LAUNCH_APP1)},
		{"LAUNCH_APP2", int(EKeyCode::LAUNCH_APP2)},
		{"OEM_1", int(EKeyCode::OEM_1)},
		{"OEM_PLUS", int(EKeyCode::OEM_PLUS)},
		{"OEM_COMMA", int(EKeyCode::OEM_COMMA)},
		{"OEM_MINUS", int(EKeyCode::OEM_MINUS)},
		{"OEM_PERIOD", int(EKeyCode::OEM_PERIOD)},
		{"OEM_2", int(EKeyCode::OEM_2)},
		{"OEM_3", int(EKeyCode::OEM_3)},
		{"GAMEPAD_A", int(EKeyCode::GAMEPAD_A)},
		{"GAMEPAD_B", int(EKeyCode::GAMEPAD_B)},
		{"GAMEPAD_X", int(EKeyCode::GAMEPAD_X)},
		{"GAMEPAD_Y", int(EKeyCode::GAMEPAD_Y)},
		{"GAMEPAD_RIGHT_SHOULDER", int(EKeyCode::GAMEPAD_RIGHT_SHOULDER)},
		{"GAMEPAD_LEFT_SHOULDER", int(EKeyCode::GAMEPAD_LEFT_SHOULDER)},
		{"GAMEPAD_LEFT_TRIGGER", int(EKeyCode::GAMEPAD_LEFT_TRIGGER)},
		{"GAMEPAD_RIGHT_TRIGGER", int(EKeyCode::GAMEPAD_RIGHT_TRIGGER)},
		{"GAMEPAD_DPAD_UP", int(EKeyCode::GAMEPAD_DPAD_UP)},
		{"GAMEPAD_DPAD_DOWN", int(EKeyCode::GAMEPAD_DPAD_DOWN)},
		{"GAMEPAD_DPAD_LEFT", int(EKeyCode::GAMEPAD_DPAD_LEFT)},
		{"GAMEPAD_DPAD_RIGHT", int(EKeyCode::GAMEPAD_DPAD_RIGHT)},
		{"GAMEPAD_MENU", int(EKeyCode::GAMEPAD_MENU)},
		{"GAMEPAD_VIEW", int(EKeyCode::GAMEPAD_VIEW)},
		{"GAMEPAD_LEFT_THUMBSTICK_BUTTON", int(EKeyCode::GAMEPAD_LEFT_THUMBSTICK_BUTTON)},
		{"GAMEPAD_RIGHT_THUMBSTICK_BUTTON", int(EKeyCode::GAMEPAD_RIGHT_THUMBSTICK_BUTTON)},
		{"GAMEPAD_LEFT_THUMBSTICK_UP", int(EKeyCode::GAMEPAD_LEFT_THUMBSTICK_UP)},
		{"GAMEPAD_LEFT_THUMBSTICK_DOWN", int(EKeyCode::GAMEPAD_LEFT_THUMBSTICK_DOWN)},
		{"GAMEPAD_LEFT_THUMBSTICK_RIGHT", int(EKeyCode::GAMEPAD_LEFT_THUMBSTICK_RIGHT)},
		{"GAMEPAD_LEFT_THUMBSTICK_LEFT", int(EKeyCode::GAMEPAD_LEFT_THUMBSTICK_LEFT)},
		{"GAMEPAD_RIGHT_THUMBSTICK_UP", int(EKeyCode::GAMEPAD_RIGHT_THUMBSTICK_UP)},
		{"GAMEPAD_RIGHT_THUMBSTICK_DOWN", int(EKeyCode::GAMEPAD_RIGHT_THUMBSTICK_DOWN)},
		{"GAMEPAD_RIGHT_THUMBSTICK_RIGHT", int(EKeyCode::GAMEPAD_RIGHT_THUMBSTICK_RIGHT)},
		{"GAMEPAD_RIGHT_THUMBSTICK_LEFT", int(EKeyCode::GAMEPAD_RIGHT_THUMBSTICK_LEFT)},
		{"OEM_4", int(EKeyCode::OEM_4)},
		{"OEM_5", int(EKeyCode::OEM_5)},
		{"OEM_6", int(EKeyCode::OEM_6)},
		{"OEM_7", int(EKeyCode::OEM_7)},
		{"OEM_8", int(EKeyCode::OEM_8)},
		{"OEM_AX", int(EKeyCode::OEM_AX)},
		{"OEM_102", int(EKeyCode::OEM_102)},
		{"ICO_HELP", int(EKeyCode::ICO_HELP)},
		{"ICO_00", int(EKeyCode::ICO_00)},
		{"PROCESSKEY", int(EKeyCode::PROCESSKEY)},
		{"ICO_CLEAR", int(EKeyCode::ICO_CLEAR)},
		{"PACKET", int(EKeyCode::PACKET)},
		{"OEM_RESET", int(EKeyCode::OEM_RESET)},
		{"OEM_JUMP", int(EKeyCode::OEM_JUMP)},
		{"OEM_PA1", int(EKeyCode::OEM_PA1)},
		{"OEM_PA2", int(EKeyCode::OEM_PA2)},
		{"OEM_PA3", int(EKeyCode::OEM_PA3)},
		{"OEM_WSCTRL", int(EKeyCode::OEM_WSCTRL)},
		{"OEM_CUSEL", int(EKeyCode::OEM_CUSEL)},
		{"OEM_ATTN", int(EKeyCode::OEM_ATTN)},
		{"OEM_FINISH", int(EKeyCode::OEM_FINISH)},
		{"OEM_COPY", int(EKeyCode::OEM_COPY)},
		{"OEM_AUTO", int(EKeyCode::OEM_AUTO)},
		{"OEM_ENLW", int(EKeyCode::OEM_ENLW)},
		{"OEM_BACKTAB", int(EKeyCode::OEM_BACKTAB)},
		{"ATTN", int(EKeyCode::ATTN)},
		{"CRSEL", int(EKeyCode::CRSEL)},
		{"EXSEL", int(EKeyCode::EXSEL)},
		{"EREOF", int(EKeyCode::EREOF)},
		{"PLAY", int(EKeyCode::PLAY)},
		{"ZOOM", int(EKeyCode::ZOOM)},
		{"NONAME", int(EKeyCode::NONAME)},
		{"PA1", int(EKeyCode::PA1)},
		{"OEM_CLEAR", int(EKeyCode::OEM_CLEAR)}
	};

	constexpr NamedCode PointerCodes[] =
	{
		{"MOUSE_POSITION", int(EPointerCode::MOUSE_POSITION)},
		{"MOUSE_DELTA", int(EPointerCode::MOUSE_DELTA)}
	};

	constexpr NamedCode GamepadCodes[] =
	{
		{"DPAD_UP", int(EGamepadCode::DPAD_UP)},
		{"DPAD_DOWN", int(EGamepadCode::DPAD_DOWN)},
		{"DPAD_LEFT", int(EGamepadCode::DPAD_LEFT)},
		{"DPAD_RIGHT", int(EGamepadCode::DPAD_RIGHT)},
		{"BUTTON_START", int(EGamepadCode::BUTTON_START)},
		{"BUTTON_BACK", int(EGamepadCode::BUTTON_BACK)},
		{"THUMB_LEFT", int(EGamepadCode::THUMB_LEFT)},
		{"THUMB_RIGHT", int(EGamepadCode::THUMB_RIGHT)},
		{"SHOULDER_LEFT", int(EGamepadCode::SHOULDER_LEFT)},
		{"SHOULDER_RIGHT", int(EGamepadCode::SHOULDER_RIGHT)},
		{"BUTTON_A", int(EGamepadCode::BUTTON_A)},
		{"BUTTON_B", int(EGamepadCode::BUTTON_B)},
		{"BUTTON_X", int(EGamepadCode::BUTTON_X)},
		{"BUTTON_Y", int(EGamepadCode::BUTTON_Y)},
		{"TRIGGER_LEFT", int(EGamepadCode::TRIGGER_LEFT)},
		{"TRIGGER_RIGHT", int(EGamepadCode::TRIGGER_RIGHT)},
		{"ANALOG_LEFT", int(EGamepadCode::ANALOG_LEFT)},
		{"ANALOG_RIGHT", int(EGamepadCode::ANALOG_RIGHT)}
	};

	[[noreturn]] void Invalid(const std::string& aField, const std::string& aReason)
	{
		throw std::runtime_error(aField + ": " + aReason);
	}

	const Json& Required(const Json& anObject, const char* aName, const std::string& aParent)
	{
		const auto found = anObject.find(aName);
		if (found == anObject.end())
		{
			Invalid(aParent + "." + aName, "missing field");
		}

		return *found;
	}

	const Json& ReadObject(const Json& anElement, const std::string& aField)
	{
		if (!anElement.is_object())
		{
			Invalid(aField, "expected an object");
		}

		return anElement;
	}

	std::string ReadString(const Json& anElement, const std::string& aField)
	{
		if (!anElement.is_string())
		{
			Invalid(aField, "expected a string");
		}

		return anElement.get<std::string>();
	}

	std::optional<std::string> ReadOptionalString(const Json& anObject, const char* aName, const std::string& aParent)
	{
		const auto found = anObject.find(aName);
		if (found == anObject.end())
		{
			return std::nullopt;
		}
		return ReadString(*found, aParent + "." + aName);
	}

	bool ReadBool(const Json& anElement, const std::string& aField)
	{
		if (!anElement.is_boolean())
		{
			Invalid(aField, "expected a boolean");
		}

		return anElement.get<bool>();
	}

	unsigned ReadDimension(const Json& anElement, const std::string& aField)
	{
		if (!anElement.is_number_integer())
		{
			Invalid(aField, "expected a positive integer");
		}
		const auto value = anElement.get<int64_t>();
		if (value <= 0 || static_cast<uint64_t>(value) > std::numeric_limits<unsigned>::max())
		{
			Invalid(aField, "expected a positive integer");
		}

		return static_cast<unsigned>(value);
	}

	float ReadVolume(const Json& anElement, const std::string& aField)
	{
		if (!anElement.is_number())
		{
			Invalid(aField, "expected a number from 0 to 1");
		}
		const double value = anElement.get<double>();
		if (!std::isfinite(value) || value < 0 || value > 1)
		{
			Invalid(aField, "expected a number from 0 to 1");
		}

		return static_cast<float>(value);
	}

	void ValidateApplication(const ApplicationSettings& applicationSettings, const std::string& aField)
	{
		if (applicationSettings.Title.empty())
		{
			Invalid(aField + ".title", "cannot be empty");
		}
		if (applicationSettings.WindowClassName.empty())
		{
			Invalid(aField + ".windowClassName", "cannot be empty");
		}

		if (!applicationSettings.WindowedWidth || !applicationSettings.WindowedHeight)
		{
			Invalid(aField + ".windowedResolution", "width and height must be positive");
		}

		if (applicationSettings.Mode != WindowMode::Windowed && applicationSettings.Mode != WindowMode::Borderless)
		{
			Invalid(aField + ".windowMode", "expected windowed or borderless");
		}

		if (applicationSettings.Resizable)
		{
			Invalid(aField + ".resizable", "true is not supported");
		}

		if (applicationSettings.ContentPath.empty() || applicationSettings.ContentPath.has_root_path())
		{
			Invalid(aField + ".contentPath", "must be a relative folder path");
		}

		// TODO: scene name having to be set in multiple places feels wrong shuold be changed.
		if (applicationSettings.InitialScene != "Blockout" &&
			applicationSettings.InitialScene != "Chests" &&
			applicationSettings.InitialScene != "ChestMaterials")
		{
			Invalid(aField + ".initialScene", "expected Blockout, Chests, or ChestMaterials");
		}
	}

	void ValidateSound(const SoundSettings& soundSettings, const std::string& aField)
	{
		const auto isValidVolume = [](float aVolume)
		{
			return std::isfinite(aVolume) && aVolume >= 0 && aVolume <= 1;
		};

		if (!isValidVolume(soundSettings.MasterVolume))
		{
			Invalid(aField + ".masterVolume", "must be from 0 to 1");
		}

		if (!isValidVolume(soundSettings.MusicVolume))
		{
			Invalid(aField + ".musicVolume", "must be from 0 to 1");
		}

		if (!isValidVolume(soundSettings.SfxVolume))
		{
			Invalid(aField + ".sfxVolume", "must be from 0 to 1");
		}
	}

	template <size_t Count>
	std::optional<int> ParseInputCode(const Json& anElement, const NamedCode (&someCodes)[Count], const std::string& aField)
	{
		if (anElement.is_null())
		{
			return std::nullopt;
		}

		const std::string name = ReadString(anElement, aField);
		for (const NamedCode& code : someCodes)
		{
			if (name == code.Name)
			{
				return code.Code;
			}
		}

		Invalid(aField, "unknown input code '" + name + "'");
	}

	template <size_t Count>
	std::optional<int> ParseOptionalInputCode(const Json& anObject, const char* aName, const NamedCode (&someCodes)[Count], const std::string& aField)
	{
		const auto found = anObject.find(aName);
		return found == anObject.end() ? std::nullopt : ParseInputCode(*found, someCodes, aField);
	}

	ApplicationSettings ParseApplication(const Json& anObject, const std::string& aField)
	{
		ApplicationSettings settings;
		settings.Title = ReadString(Required(anObject, "title", aField), aField + ".title");
		if (const auto className = ReadOptionalString(anObject, "windowClassName", aField))
		{
			settings.WindowClassName = *className;
		}

		const std::string resolutionField = aField + ".windowedResolution";
		const Json& resolution = ReadObject(Required(anObject, "windowedResolution", aField), resolutionField);
		settings.WindowedWidth = ReadDimension(Required(resolution, "width", resolutionField), resolutionField + ".width");
		settings.WindowedHeight = ReadDimension(Required(resolution, "height", resolutionField), resolutionField + ".height");

		const std::string mode = ReadString(Required(anObject, "windowMode", aField), aField + ".windowMode");
		if (mode == "windowed")
		{
			settings.Mode = WindowMode::Windowed;
		}
		else if (mode == "borderless")
		{
			settings.Mode = WindowMode::Borderless;
		}
		else
		{
			Invalid(aField + ".windowMode", "expected windowed or borderless");
		}

		settings.Resizable = ReadBool(Required(anObject, "resizable", aField), aField + ".resizable");
		settings.EnableRenderDiagnostics = ReadBool(Required(anObject, "renderDiagnostics", aField), aField + ".renderDiagnostics");
		settings.EnableMouseLook = ReadBool(Required(anObject, "mouseLook", aField), aField + ".mouseLook");

		const std::string contentPathUtf8 = ReadString(Required(anObject, "contentPath", aField), aField + ".contentPath");
		const std::u8string contentPath(reinterpret_cast<const char8_t*>(contentPathUtf8.data()), contentPathUtf8.size());
		settings.ContentPath = std::filesystem::path(contentPath);

		if (const auto cursorPath = ReadOptionalString(anObject, "cursorPath", aField))
		{
			const std::u8string utf8Path(reinterpret_cast<const char8_t*>(cursorPath->data()), cursorPath->size());
			settings.CursorPath = std::filesystem::path(utf8Path);
		}

		// Older settings files start with Blockout if this field is absent.
		if (const auto initialScene = ReadOptionalString(anObject, "initialScene", aField))
		{
			settings.InitialScene = *initialScene;
		}

		ValidateApplication(settings, aField);
		return settings;
	}

	SoundSettings ParseSound(const Json& anObject, const std::string& aField)
	{
		SoundSettings sound;
		sound.MasterVolume = ReadVolume(Required(anObject, "masterVolume", aField), aField + ".masterVolume");
		sound.MusicVolume = ReadVolume(Required(anObject, "musicVolume", aField), aField + ".musicVolume");
		sound.SfxVolume = ReadVolume(Required(anObject, "sfxVolume", aField), aField + ".sfxVolume");

		return sound;
	}

	InputSettings ParseInput(const Json& anObject, const std::string& aField)
	{
		InputSettings settings;
		for (auto entry = anObject.begin(); entry != anObject.end(); ++entry)
		{
			const std::string action = entry.key();
			if (action.empty())
			{
				Invalid(aField, "action name cannot be empty");
			}

			const std::string actionField = aField + "." + action;
			const Json& codes = ReadObject(entry.value(), actionField);
			const auto key = ParseOptionalInputCode(codes, "key", KeyCodes, actionField + ".key");
			const auto pointer = ParseOptionalInputCode(codes, "pointer", PointerCodes, actionField + ".pointer");
			const auto gamepad = ParseOptionalInputCode(codes, "gamepad", GamepadCodes, actionField + ".gamepad");

			ActionBindings binding;
			if (key)
			{
				binding.Key = static_cast<EKeyCode>(*key);
			}
			if (pointer)
			{
				binding.Pointer = static_cast<EPointerCode>(*pointer);
			}
			if (gamepad)
			{
				binding.Gamepad = static_cast<EGamepadCode>(*gamepad);
			}

			settings.Actions.emplace(action, binding);
		}

		return settings;
	}

	template <size_t Count>
	const char* InputCodeName(int anInputCode, const NamedCode (&someCodes)[Count], const std::string& aField)
	{
		for (const NamedCode& code : someCodes)
		{
			if (code.Code == anInputCode)
			{
				return code.Name;
			}
		}
		Invalid(aField, "unknown input code");
	}

	void ValidateInput(const InputSettings& someSettings, const std::string& aField)
	{
		for (const auto& [action, binding] : someSettings.Actions)
		{
			if (action.empty())
			{
				Invalid(aField, "action name cannot be empty");
			}
			const std::string actionField = aField + "." + action;
			if (binding.Key)
			{
				InputCodeName(int(*binding.Key), KeyCodes, actionField + ".key");
			}
			if (binding.Pointer)
			{
				InputCodeName(int(*binding.Pointer), PointerCodes, actionField + ".pointer");
			}
			if (binding.Gamepad)
			{
				InputCodeName(int(*binding.Gamepad), GamepadCodes, actionField + ".gamepad");
			}
		}
	}

	std::string PathUtf8(const std::filesystem::path& aPath)
	{
		const std::u8string encoded = aPath.generic_u8string();
		return std::string(reinterpret_cast<const char*>(encoded.data()), encoded.size());
	}

	void SetApplicationJson(Json& section, const ApplicationSettings& settings)
	{
		section["title"] = settings.Title;
		section["windowClassName"] = settings.WindowClassName;
		section["windowedResolution"]["width"] = settings.WindowedWidth;
		section["windowedResolution"]["height"] = settings.WindowedHeight;
		section["windowMode"] = settings.Mode == WindowMode::Windowed ? "windowed" : "borderless";
		section["resizable"] = settings.Resizable;
		section["renderDiagnostics"] = settings.EnableRenderDiagnostics;
		section["mouseLook"] = settings.EnableMouseLook;
		section["contentPath"] = PathUtf8(settings.ContentPath);
		section["cursorPath"] = PathUtf8(settings.CursorPath);
		section["initialScene"] = settings.InitialScene;
	}

	void SetSoundJson(Json& section, const SoundSettings& settings)
	{
		// Avoid exposing binary float noise such as 0.349999994 in editable JSON.
		const auto readableVolume = [](float volume)
		{
			return std::round(static_cast<double>(volume) * 1'000'000.0) / 1'000'000.0;
		};
		section["masterVolume"] = readableVolume(settings.MasterVolume);
		section["musicVolume"] = readableVolume(settings.MusicVolume);
		section["sfxVolume"] = readableVolume(settings.SfxVolume);
	}

	void SetInputJson(Json& section, const InputSettings& settings)
	{
		std::vector<std::string> removedActions;
		for (auto action = section.begin(); action != section.end(); ++action)
		{
			if (!settings.Actions.contains(action.key()))
			{
				removedActions.push_back(action.key());
			}
		}
		for (const std::string& action : removedActions)
		{
			section.erase(action);
		}

		for (const auto& [action, binding] : settings.Actions)
		{
			Json& codes = section[action];
			if (codes.is_null())
			{
				codes = Json::object();
			}
			ReadObject(codes, "InputBindings.json.current." + action);
			codes["key"] = binding.Key ? Json(InputCodeName(int(*binding.Key), KeyCodes, action + ".key")) : Json(nullptr);
			codes["pointer"] = binding.Pointer ? Json(InputCodeName(int(*binding.Pointer), PointerCodes, action + ".pointer")) : Json(nullptr);
			codes["gamepad"] = binding.Gamepad ? Json(InputCodeName(int(*binding.Gamepad), GamepadCodes, action + ".gamepad")) : Json(nullptr);
		}
	}

	Json ReadSettingsFile(const std::filesystem::path& aPath)
	{
		std::ifstream file(aPath, std::ios::binary);
		if (!file)
		{
			Invalid(aPath.filename().string(), "cannot read file");
		}
		try
		{
			return Json::parse(file);
		}
		catch (const Json::parse_error& error)
		{
			Invalid(aPath.filename().string(), std::string("malformed JSON: ") + error.what());
		}
	}

	Json& CurrentObject(Json& aDocument, const std::string& aFileName)
	{
		ReadObject(aDocument, aFileName);
		ReadObject(Required(aDocument, "current", aFileName), aFileName + ".current");
		return aDocument.at("current");
	}

	Json& CurrentSection(Json& aDocument, const char* aSection, const std::string& aFileName)
	{
		Json& current = CurrentObject(aDocument, aFileName);
		ReadObject(Required(current, aSection, aFileName + ".current"), aFileName + ".current." + aSection);
		return current.at(aSection);
	}

	void SaveSettingsFile(const std::filesystem::path& aPath, const Json& aDocument)
	{
		const std::filesystem::path temporaryPath = aPath.wstring() + L".tmp";
		try
		{
			const std::string text = aDocument.dump(2) + '\n';
			{
				std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
				if (!file || !file.write(text.data(), static_cast<std::streamsize>(text.size())) || !file.flush())
				{
					throw std::runtime_error("could not write temporary file");
				}
			}
			if (!MoveFileExW(temporaryPath.c_str(), aPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			{
				throw std::runtime_error("could not replace settings file (Windows error " + std::to_string(GetLastError()) + ")");
			}
		}
		catch (const std::exception& error)
		{
			std::error_code ignored;
			std::filesystem::remove(temporaryPath, ignored);
			throw std::runtime_error(aPath.filename().string() + ": " + error.what());
		}
		catch (...)
		{
			std::error_code ignored;
			std::filesystem::remove(temporaryPath, ignored);
			throw;
		}
	}

}

EngineSettings::EngineSettings(std::filesystem::path anExecutableDirectory, std::filesystem::path aSettingsDirectory)
	: myExecutableDirectory(std::move(anExecutableDirectory))
	, mySettingsDirectory(std::move(aSettingsDirectory))
{
}

void EngineSettings::Load()
{
	const std::filesystem::path applicationFile = mySettingsDirectory / "ApplicationSettings.json";
	const std::filesystem::path inputFile = mySettingsDirectory / "InputBindings.json";

	// Parse both files before replacing any live settings.
	try
	{
		const Json applicationRoot = ReadObject(ReadSettingsFile(applicationFile), "ApplicationSettings.json");
		const Json& defaults = ReadObject(Required(applicationRoot, "defaults", "ApplicationSettings.json"), "ApplicationSettings.json.defaults");
		const Json& current = ReadObject(Required(applicationRoot, "current", "ApplicationSettings.json"), "ApplicationSettings.json.current");

		const Json& defaultApplicationObject = ReadObject(Required(defaults, "application", "ApplicationSettings.json.defaults"), "ApplicationSettings.json.defaults.application");
		const Json& defaultSoundObject = ReadObject(Required(defaults, "sound", "ApplicationSettings.json.defaults"), "ApplicationSettings.json.defaults.sound");
		const Json& currentApplicationObject = ReadObject(Required(current, "application", "ApplicationSettings.json.current"), "ApplicationSettings.json.current.application");
		const Json& currentSoundObject = ReadObject(Required(current, "sound", "ApplicationSettings.json.current"), "ApplicationSettings.json.current.sound");

		ApplicationSettings defaultApplication = ParseApplication(defaultApplicationObject, "ApplicationSettings.json.defaults.application");
		SoundSettings defaultSound = ParseSound(defaultSoundObject, "ApplicationSettings.json.defaults.sound");
		ApplicationSettings currentApplication = ParseApplication(currentApplicationObject, "ApplicationSettings.json.current.application");
		SoundSettings currentSound = ParseSound(currentSoundObject, "ApplicationSettings.json.current.sound");

		const Json inputRoot = ReadObject(ReadSettingsFile(inputFile), "InputBindings.json");
		const Json& defaultInputObject = ReadObject(Required(inputRoot, "defaults", "InputBindings.json"), "InputBindings.json.defaults");
		const Json& currentInputObject = ReadObject(Required(inputRoot, "current", "InputBindings.json"), "InputBindings.json.current");
		InputSettings defaultInput = ParseInput(defaultInputObject, "InputBindings.json.defaults");
		InputSettings currentInput = ParseInput(currentInputObject, "InputBindings.json.current");

		// All sections are valid; publish them together.
		myDefaultApplication = std::move(defaultApplication);
		myCurrentApplication = std::move(currentApplication);
		myDefaultSound = defaultSound;
		myCurrentSound = currentSound;
		myDefaultInput = std::move(defaultInput);
		myCurrentInput = std::move(currentInput);
	}
	catch (const std::exception& error)
	{
		throw std::runtime_error("Settings load failed in " + mySettingsDirectory.string() + ": " + error.what());
	}
}

std::filesystem::path EngineSettings::GetContentRoot() const
{
	const std::filesystem::path contentRoot = (myExecutableDirectory / myCurrentApplication.ContentPath).lexically_normal();
	if (!std::filesystem::is_directory(contentRoot))
	{
		throw std::runtime_error("ApplicationSettings.json.current.application.contentPath: content folder does not exist: " + contentRoot.string());
	}
	return std::filesystem::canonical(contentRoot);
}

std::vector<WindowResolution> EngineSettings::GetAvailableWindowedResolutions() const
{
	if (myAvailableResolutions)
	{
		return myAvailableResolutions();
	}

	return {};
}

void EngineSettings::UpdateApplicationSettings(const ApplicationSettings& applicationSettings)
{
	ValidateApplication(applicationSettings, "ApplicationSettings.json.current.application");
	const std::filesystem::path file = mySettingsDirectory / "ApplicationSettings.json";
	Json document = ReadSettingsFile(file);
	Json& section = CurrentSection(document, "application", "ApplicationSettings.json");
	ReadObject(Required(section, "windowedResolution", "ApplicationSettings.json.current.application"), "ApplicationSettings.json.current.application.windowedResolution");
	try
	{
		ApplicationSettings appliedSettings = myApplicationApply ? myApplicationApply(applicationSettings) : applicationSettings;
		ValidateApplication(appliedSettings, "ApplicationSettings.json.current.application");
		SetApplicationJson(section, appliedSettings);
		SaveSettingsFile(file, document);
		myCurrentApplication = std::move(appliedSettings);
	}
	catch (...)
	{
		if (myApplicationApply)
		{
			try
			{
				myApplicationApply(myCurrentApplication);
			}
			catch (...)
			{
				// Keep the original error.
			}
		}
		throw;
	}
}

void EngineSettings::UpdateSoundSettings(const SoundSettings& soundSettings)
{
	ValidateSound(soundSettings, "ApplicationSettings.json.current.sound");
	const std::filesystem::path file = mySettingsDirectory / "ApplicationSettings.json";
	Json document = ReadSettingsFile(file);
	SetSoundJson(CurrentSection(document, "sound", "ApplicationSettings.json"), soundSettings);
	try
	{
		if (mySoundApply)
		{
			mySoundApply(soundSettings);
		}
		SaveSettingsFile(file, document);
		myCurrentSound = soundSettings;
	}
	catch (...)
	{
		if (mySoundApply)
		{
			try
			{
				mySoundApply(myCurrentSound);
			}
			catch (...)
			{
				// Keep the original error.
			}
		}
		throw;
	}
}

void EngineSettings::UpdateInputSettings(const InputSettings& inputSettings)
{
	ValidateInput(inputSettings, "InputBindings.json.current");
	InputSettings staged = inputSettings;
	const std::filesystem::path file = mySettingsDirectory / "InputBindings.json";
	Json document = ReadSettingsFile(file);
	SetInputJson(CurrentObject(document, "InputBindings.json"), inputSettings);
	try
	{
		if (myInputApply)
		{
			myInputApply(inputSettings);
		}
		SaveSettingsFile(file, document);
		myCurrentInput = std::move(staged);
	}
	catch (...)
	{
		if (myInputApply)
		{
			try
			{
				myInputApply(myCurrentInput);
			}
			catch (...)
			{
				// Keep the original error.
			}
		}
		throw;
	}
}

void EngineSettings::ResetApplicationSettings()
{
	UpdateApplicationSettings(myDefaultApplication);
}

void EngineSettings::ResetSoundSettings()
{
	UpdateSoundSettings(myDefaultSound);
}

void EngineSettings::ResetInputSettings()
{
	UpdateInputSettings(myDefaultInput);
}

void EngineSettings::ResetAllSettings()
{
	const ApplicationSettings previousApplication = myCurrentApplication;
	const SoundSettings previousSound = myCurrentSound;
	try
	{
		ResetApplicationSettings();
		ResetSoundSettings();
		ResetInputSettings();
	}
	catch (...)
	{
		try
		{
			UpdateSoundSettings(previousSound);
		}
		catch (...)
		{
			// Keep the original error.
		}
		try
		{
			UpdateApplicationSettings(previousApplication);
		}
		catch (...)
		{
			// Keep the original error.
		}
		throw;
	}
}
