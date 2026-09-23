#include "GameFramework/Runtime/InputSystem.h"
#include "EnumKeyCode.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
	constexpr int VirtualKeyCount = 256;

	bool IsKeyDown(const InputDeviceFrame& frame, int key)
	{
		return key >= 0 && key < VirtualKeyCount && frame.KeysDown[static_cast<size_t>(key)];
	}
}

struct InputSubscription::SharedState
{
	struct Listener
	{
		size_t Id;
		std::string Action;
		InputSystem::Callback Callback;
		bool Active = true;
		bool PendingRemove = false;
	};
	std::vector<Listener> Listeners;
	size_t NextId = 0;
	bool Dispatching = false;
};

InputActionId::InputActionId(std::string name) : myName(std::move(name))
{
	if (myName.empty())
	{
		throw std::invalid_argument("Input action name cannot be empty");
	}
}

InputSubscription::~InputSubscription() { Reset(); }
InputSubscription::InputSubscription(InputSubscription&& other) noexcept
	: myState(std::move(other.myState)), myListener(other.myListener)
{
	other.myListener = 0;
}
InputSubscription& InputSubscription::operator=(InputSubscription&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		myState = std::move(other.myState);
		myListener = other.myListener;
		other.myListener = 0;
	}
	return *this;
}
void InputSubscription::Reset()
{
	if (myListener != 0)
	{
		if (std::shared_ptr<SharedState> state = myState.lock())
		{
			for (SharedState::Listener& listener : state->Listeners)
			{
				if (listener.Id != myListener)
				{
					continue;
				}
				if (state->Dispatching)
				{
					// Removal takes effect after dispatch; later listeners still receive this event.
					listener.PendingRemove = true;
				}
				else
				{
					listener.Active = false;
				}
				break;
			}
		}
	}
	myListener = 0;
	myState.reset();
}

InputSystem::InputSystem() : myShared(std::make_shared<InputSubscription::SharedState>()) {}
InputSystem::~InputSystem() = default;

InputSubscription InputSystem::Subscribe(const InputActionId& action, Callback callback)
{
	if (!callback)
	{
		throw std::invalid_argument("Input callback cannot be empty");
	}
	const size_t id = ++myShared->NextId;
	myShared->Listeners.push_back({id, action.Name(), std::move(callback), true, false});
	return {myShared, id};
}

void InputSystem::BindKey(const InputActionId& action, int key, std::vector<int> required, std::vector<int> forbidden)
{
	if (key < 0 || key >= VirtualKeyCount)
	{
		throw std::out_of_range("Key binding is outside the virtual-key range");
	}
	myKeys.push_back({action, key, std::move(required), std::move(forbidden)});
}
void InputSystem::BindMouseDelta(const InputActionId& action, float scale) { myMouse.push_back({action, scale}); }
void InputSystem::BindGamepadButton(const InputActionId& action, unsigned button) { myGamepadButtons.push_back({action, button}); }
void InputSystem::BindGamepadAxis2D(const InputActionId& action, bool right, float scale) { myGamepadAxes.push_back({action, right, scale}); }
void InputSystem::BindGamepadTrigger(const InputActionId& action, bool right, float scale) { myGamepadTriggers.push_back({action, right, scale}); }

void InputSystem::ClearBindings()
{
	myKeys.clear();
	myMouse.clear();
	myGamepadButtons.clear();
	myGamepadAxes.clear();
	myGamepadTriggers.clear();
	Reset();
}

void InputSystem::Dispatch(const InputActionId& action, InputActionPhase phase, const InputActionValue& value)
{
	const auto removeInactiveListeners = [this]
	{
		for (InputSubscription::SharedState::Listener& listener : myShared->Listeners)
		{
			if (listener.PendingRemove)
			{
				listener.Active = false;
			}
		}
		std::erase_if(myShared->Listeners, [](const InputSubscription::SharedState::Listener& listener)
		{
			return !listener.Active;
		});
	};

	myShared->Dispatching = true;
	const size_t listenerCountAtDispatchStart = myShared->Listeners.size();
	const InputActionEvent event{action, phase, value};
	try
	{
		for (size_t listenerIndex = 0; listenerIndex < listenerCountAtDispatchStart; ++listenerIndex)
		{
			InputSubscription::SharedState::Listener& listener = myShared->Listeners[listenerIndex];
			if (listener.Active && listener.Action == action.Name())
			{
				listener.Callback(event);
			}
		}
	}
	catch (...)
	{
		myShared->Dispatching = false;
		removeInactiveListeners();
		throw;
	}
	myShared->Dispatching = false;
	removeInactiveListeners();
}

bool InputSystem::IsKeyBindingPressed(const KeyBinding& binding, const InputDeviceFrame& frame)
{
	if (!IsKeyDown(frame, binding.Key))
	{
		return false;
	}
	for (int modifier : binding.Required)
	{
		if (!IsKeyDown(frame, modifier))
		{
			return false;
		}
	}
	for (int modifier : binding.Forbidden)
	{
		if (IsKeyDown(frame, modifier))
		{
			return false;
		}
	}
	return true;
}

bool InputSystem::HasPressedMoreSpecificBinding(const KeyBinding& binding, const InputDeviceFrame& frame) const
{
	for (const KeyBinding& candidate : myKeys)
	{
		if (candidate.Key != binding.Key || candidate.Required.size() <= binding.Required.size())
		{
			continue;
		}
		if (!IsKeyBindingPressed(candidate, frame))
		{
			continue;
		}

		bool includesRequiredModifiers = true;
		for (int modifier : binding.Required)
		{
			if (std::find(candidate.Required.begin(), candidate.Required.end(), modifier) == candidate.Required.end())
			{
				includesRequiredModifiers = false;
				break;
			}
		}
		if (includesRequiredModifiers)
		{
			return true;
		}
	}
	return false;
}

void InputSystem::Update(const InputDeviceFrame& frame)
{
	std::unordered_map<std::string, InputActionValue> values;
	std::unordered_map<std::string, InputActionId> ids;

	// Collect focused bindings in device order; the last active value wins.
	if (frame.Focused)
	{
		for (const KeyBinding& binding : myKeys)
		{
			bool active = IsKeyBindingPressed(binding, frame);
			if (active && HasPressedMoreSpecificBinding(binding, frame))
			{
				active = false;
			}
			ids.emplace(binding.Action.Name(), binding.Action);
			if (active)
			{
				values[binding.Action.Name()] = true;
			}
		}
		for (const MouseBinding& binding : myMouse)
		{
			ids.emplace(binding.Action.Name(), binding.Action);
			const CommonUtilities::Vector2f scaledDelta = frame.MouseDelta * binding.Scale;
			if (scaledDelta.LengthSqr() > 0)
			{
				values[binding.Action.Name()] = scaledDelta;
			}
		}
		for (const GamepadButtonBinding& binding : myGamepadButtons)
		{
			ids.emplace(binding.Action.Name(), binding.Action);
			const auto button = frame.GamepadButtonsDown.find(binding.Button);
			if (button != frame.GamepadButtonsDown.end() && button->second)
			{
				values[binding.Action.Name()] = true;
			}
		}
		for (const GamepadAxisBinding& binding : myGamepadAxes)
		{
			ids.emplace(binding.Action.Name(), binding.Action);
			const CommonUtilities::Vector2f scaledAxis = (binding.Right ? frame.GamepadRight : frame.GamepadLeft) * binding.Scale;
			if (scaledAxis.LengthSqr() > 0)
			{
				values[binding.Action.Name()] = scaledAxis;
			}
		}
		for (const GamepadTriggerBinding& binding : myGamepadTriggers)
		{
			ids.emplace(binding.Action.Name(), binding.Action);
			const float value = (binding.Right ? frame.GamepadRightTrigger : frame.GamepadLeftTrigger) * binding.Scale;
			if (value != 0)
			{
				values[binding.Action.Name()] = value;
			}
		}
	}

	// Include previously tracked actions so focus loss can end them, then sort by name.
	for (const auto& [name, id] : ids)
	{
		myActions.try_emplace(name);
	}
	std::vector<std::string> orderedActions;
	orderedActions.reserve(myActions.size());
	for (const auto& [name, state] : myActions)
	{
		orderedActions.push_back(name);
	}
	std::sort(orderedActions.begin(), orderedActions.end());

	// Dispatch before committing state; a throwing callback leaves this action unchanged.
	for (const std::string& name : orderedActions)
	{
		ActionState& state = myActions.at(name);
		const auto foundValue = values.find(name);
		const bool hasValue = foundValue != values.end();
		const InputActionValue value = hasValue ? foundValue->second : state.Value;
		const auto foundId = ids.find(name);
		const InputActionId action = foundId != ids.end() ? foundId->second : InputActionId{name};
		if (hasValue && !state.Active)
		{
			Dispatch(action, InputActionPhase::Started, value);
		}
		else if (hasValue)
		{
			Dispatch(action, InputActionPhase::Ongoing, value);
		}
		else if (state.Active)
		{
			Dispatch(action, InputActionPhase::Ended, state.Value);
		}
		state.Active = hasValue;
		if (hasValue)
		{
			state.Value = value;
		}
	}
}

void InputSystem::Reset()
{
	myActions.clear();
}

namespace InputActions
{
	const InputActionId Quit{"Quit"},
		ReloadScene{"ReloadScene"},
		SpawnDemo{"SpawnDemo"},
		AttachDemoChild{"AttachDemoChild"},
		DebugCamera{"DebugCamera"},
		PreviousRenderPass{"PreviousRenderPass"},
		NextRenderPass{"NextRenderPass"},
		PrintDiagnostics{"PrintDiagnostics"};
	const InputActionId CameraLookEnable{"CameraLookEnable"},
		CameraLookDelta{"CameraLookDelta"},
		CameraForward{"CameraForward"},
		CameraBack{"CameraBack"},
		CameraLeft{"CameraLeft"},
		CameraRight{"CameraRight"},
		CameraUp{"CameraUp"},
		CameraDown{"CameraDown"};
	const InputActionId ToggleSpin{"ToggleSpin"},
		PlayBreathing{"PlayBreathing"},
		PlayWalk{"PlayWalk"},
		PlayRun{"PlayRun"},
		PlayWave{"PlayWave"};
	const InputActionId ToggleDirectional{"ToggleDirectional"},
		TogglePoint{"TogglePoint"},
		ToggleSpot{"ToggleSpot"},
		AimDirectional{"AimDirectional"},
		PlacePoint{"PlacePoint"},
		PlaceSpot{"PlaceSpot"},
		PrintLights{"PrintLights"};
	const InputActionId ToggleTonemapping{"ToggleTonemapping"},
		SelectACES{"SelectACES"},
		SelectLottes{"SelectLottes"},
		SelectUnrealTonemapper{"SelectUnrealTonemapper"};
}

void InstallDefaultInputBindings(InputSystem& input)
{
	using K = EKeyCode;
	const std::vector<int> anyShift{int(K::SHIFT), int(K::LSHIFT), int(K::RSHIFT)};
	input.BindKey(InputActions::Quit, int(K::ESCAPE));
	input.BindKey(InputActions::ReloadScene, int(K::F4));
	input.BindKey(InputActions::SpawnDemo, int(K::F7));
	input.BindKey(InputActions::AttachDemoChild, int(K::F8));
	input.BindKey(InputActions::DebugCamera, int(K::F1));
	input.BindKey(InputActions::PreviousRenderPass, int(K::F5));
	input.BindKey(InputActions::NextRenderPass, int(K::F6));
	input.BindKey(InputActions::PrintDiagnostics, int(K::P));
	input.BindKey(InputActions::CameraLookEnable, int(K::MOUSERBUTTON));
	input.BindMouseDelta(InputActions::CameraLookDelta);
	input.BindKey(InputActions::CameraForward, int(K::W));
	input.BindKey(InputActions::CameraBack, int(K::S));
	input.BindKey(InputActions::CameraLeft, int(K::A));
	input.BindKey(InputActions::CameraRight, int(K::D));
	input.BindKey(InputActions::CameraUp, int(K::SPACE));
	input.BindKey(InputActions::CameraDown, int(K::CONTROL));
	input.BindKey(InputActions::ToggleSpin, int(K::R));
	input.BindKey(InputActions::PlayBreathing, int(K::NUMPAD0));
	input.BindKey(InputActions::PlayWalk, int(K::NUMPAD1));
	input.BindKey(InputActions::PlayRun, int(K::NUMPAD2));
	input.BindKey(InputActions::PlayWave, int(K::NUMPAD3));
	input.BindKey(InputActions::ToggleTonemapping, int(K::NUMPAD4));
	input.BindKey(InputActions::SelectACES, int(K::NUMPAD5));
	input.BindKey(InputActions::SelectLottes, int(K::NUMPAD6));
	input.BindKey(InputActions::SelectUnrealTonemapper, int(K::NUMPAD7));
	input.BindKey(InputActions::PrintLights, int(K::P));

	// Directional light uses regular 7; Numpad7 selects the Unreal tonemapper.
	input.BindKey(InputActions::ToggleDirectional, int('7'), {}, anyShift);
	for (int modifier : anyShift)
	{
		input.BindKey(InputActions::AimDirectional, int('7'), {modifier});
	}

	// Point light: unmodified toggles, shifted places.
	input.BindKey(InputActions::TogglePoint, int(K::NUMPAD8), {}, anyShift);
	input.BindKey(InputActions::TogglePoint, int('8'), {}, anyShift);
	for (int modifier : anyShift)
	{
		input.BindKey(InputActions::PlacePoint, int(K::NUMPAD8), {modifier});
		input.BindKey(InputActions::PlacePoint, int('8'), {modifier});
	}

	// Spot light: unmodified toggles, shifted places.
	input.BindKey(InputActions::ToggleSpot, int(K::NUMPAD9), {}, anyShift);
	input.BindKey(InputActions::ToggleSpot, int('9'), {}, anyShift);
	for (int modifier : anyShift)
	{
		input.BindKey(InputActions::PlaceSpot, int(K::NUMPAD9), {modifier});
		input.BindKey(InputActions::PlaceSpot, int('9'), {modifier});
	}
}
