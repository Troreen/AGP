#pragma once
#include "Vector2.hpp"
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class InputActionId
{
public:
	explicit InputActionId(std::string name);
	const std::string& Name() const { return myName; }
	bool operator==(const InputActionId&) const = default;
private:
	std::string myName;
};

enum class InputActionPhase { Started, Ongoing, Ended };
using InputActionValue = std::variant<bool, float, CommonUtilities::Vector2f>;

struct InputActionEvent
{
	const InputActionId& Action;
	InputActionPhase Phase;
	InputActionValue Value;
};

// Device state sampled exactly once by the platform host. Public for deterministic
// tests and alternate platform backends; gameplay consumes actions, never this type.
struct InputDeviceFrame
{
	std::array<bool, 256> KeysDown{};
	CommonUtilities::Vector2f MouseDelta{};
	std::unordered_map<unsigned, bool> GamepadButtonsDown;
	CommonUtilities::Vector2f GamepadLeft{};
	CommonUtilities::Vector2f GamepadRight{};
	float GamepadLeftTrigger = 0;
	float GamepadRightTrigger = 0;
	bool Focused = true;
};

class InputSubscription
{
public:
	InputSubscription() = default;
	~InputSubscription();
	InputSubscription(InputSubscription&& other) noexcept;
	InputSubscription& operator=(InputSubscription&& other) noexcept;
	InputSubscription(const InputSubscription&) = delete;
	InputSubscription& operator=(const InputSubscription&) = delete;
	void Reset();
private:
	struct SharedState;
	std::weak_ptr<SharedState> myState;
	size_t myListener = 0;
	InputSubscription(std::weak_ptr<SharedState> state, size_t listener) : myState(std::move(state)), myListener(listener) {}
	friend class InputSystem;
};

class InputSystem
{
public:
	using Callback = std::function<void(const InputActionEvent&)>;
	InputSystem();
	~InputSystem();
	InputSystem(const InputSystem&) = delete;
	InputSystem& operator=(const InputSystem&) = delete;

	InputSubscription Subscribe(const InputActionId& action, Callback callback);
	void BindKey(const InputActionId& action, int key, std::vector<int> requiredModifiers = {}, std::vector<int> forbiddenModifiers = {});
	void BindMouseDelta(const InputActionId& action, float scale = 1);
	void BindGamepadButton(const InputActionId& action, unsigned button);
	void BindGamepadAxis2D(const InputActionId& action, bool rightStick, float scale = 1);
	void BindGamepadTrigger(const InputActionId& action, bool rightTrigger, float scale = 1);
	void ClearBindings();
	void Update(const InputDeviceFrame& frame);
	void Reset();

private:
	struct KeyBinding { InputActionId Action; int Key; std::vector<int> Required; std::vector<int> Forbidden; };
	struct MouseBinding { InputActionId Action; float Scale; };
	struct GamepadButtonBinding { InputActionId Action; unsigned Button; };
	struct GamepadAxisBinding { InputActionId Action; bool Right; float Scale; };
	struct GamepadTriggerBinding { InputActionId Action; bool Right; float Scale; };
	struct ActionState { bool Active = false; InputActionValue Value = false; };
	std::shared_ptr<InputSubscription::SharedState> myShared;
	std::vector<KeyBinding> myKeys;
	std::vector<MouseBinding> myMouse;
	std::vector<GamepadButtonBinding> myGamepadButtons;
	std::vector<GamepadAxisBinding> myGamepadAxes;
	std::vector<GamepadTriggerBinding> myGamepadTriggers;
	std::unordered_map<std::string, ActionState> myActions;
	void Dispatch(const InputActionId& action, InputActionPhase phase, const InputActionValue& value);
};

namespace InputActions
{
	extern const InputActionId Quit, ReloadScene, SpawnDemo, DebugCamera, CycleRenderPass, PrintDiagnostics;
	extern const InputActionId CameraLookEnable, CameraLookDelta, CameraForward, CameraBack, CameraLeft, CameraRight, CameraUp, CameraDown;
	extern const InputActionId ToggleSpin, PlayBreathing, PlayWalk, PlayRun, PlayWave;
	extern const InputActionId ToggleDirectional, TogglePoint, ToggleSpot, AimDirectional, PlacePoint, PlaceSpot, PrintLights;
}

void InstallDefaultInputBindings(InputSystem& input);
