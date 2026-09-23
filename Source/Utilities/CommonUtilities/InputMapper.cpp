#include "InputMapper.h"
#include "InputHandler.h"
#include "XInputHandler.h"
#include "Vector2.hpp"
#include <type_traits>
#include <cassert>

namespace CommonUtilities
{
	InputMapper::InputMapper()
		: mySharedInputHandler(nullptr)
		, mySharedXInputHandler(nullptr)
		, myEventListenerCreationCount(0u)
		, myPreviousLTriggerWasNonZero(false)
		, myPreviousRTriggerWasNonZero(false)
		, myPreviousLThumbWasNonZero(false)
		, myPreviousRThumbWasNonZero(false)
	{}

	void InputMapper::Init(CommonUtilities::InputHandler* anInputHandler, CommonUtilities::XInputHandler* anXInputHandler)
	{
		mySharedInputHandler = anInputHandler;
		mySharedXInputHandler = anXInputHandler;
	}

	void InputMapper::BindActionToInputCode(std::string_view anActionType, EKeyCode aKeyCode)
	{
		UpdateInputMapEntry(myInputMap, anActionType, static_cast<int>(aKeyCode));
	}

	void InputMapper::BindActionToInputCode(std::string_view anActionType, EPointerCode aPointerCode)
	{
		UpdateInputMapEntry(myPointerMap, anActionType, static_cast<int>(aPointerCode));
	}

	void InputMapper::BindActionToInputCode(std::string_view anActionType, EGamepadCode aGamepadCode)
	{
		UpdateInputMapEntry(myXInputMap, anActionType, static_cast<int>(aGamepadCode));
	}

	void InputMapper::ClearBindingsFromAction(std::string_view anActionType)
	{
		const size_t actionHash = std::hash<std::string_view>{}(anActionType);

		std::unordered_map<size_t, int>* const inputMapList[]
		{
			&myInputMap,
			&myXInputMap,
			&myPointerMap
		};

		for (auto* inputMap : inputMapList)
		{
			if (inputMap->empty())
			{
				continue;
			}

			for (auto it = inputMap->begin(); it != inputMap->end(); ++it)
			{
				if (it->first == actionHash)
				{
					inputMap->erase(it);
					return;
				}
			}
		}
	}

	void InputMapper::Update()
	{
		assert(mySharedInputHandler && "InputMapper::Update() was called before initializing it with an InputHandler");
		UpdateMouseAndKeyboard();

		if (mySharedXInputHandler)
		{
			UpdateGamepad();
		}
	}

	const CommonUtilities::InputHandler* InputMapper::GetInputHandler() const
	{
		return mySharedInputHandler;
	}

	CommonUtilities::InputHandler* InputMapper::GetInputHandler()
	{
		return mySharedInputHandler;
	}

	const CommonUtilities::XInputHandler* InputMapper::GetXInputHandler() const
	{
		return mySharedXInputHandler;
	}

	CommonUtilities::XInputHandler* InputMapper::GetXInputHandler()
	{
		return mySharedXInputHandler;
	}

	unsigned InputMapper::AddEventListener(std::string_view anActionType, std::function<void(const InputEvent&)> aCallback)
	{
		const size_t actionHash = std::hash<std::string_view>{}(anActionType);
		myEventListeners.emplace(actionHash, std::make_pair(++myEventListenerCreationCount, std::move(aCallback)));
		return myEventListenerCreationCount;
	}

	void InputMapper::RemoveEventListener(unsigned anEventListenerID)
	{
		for (auto it = myEventListeners.begin(); it != myEventListeners.end(); ++it)
		{
			if (it->second.first == anEventListenerID)
			{
				myEventListeners.erase(it);
				return;
			}
		}
	}

	void InputMapper::UpdateInputMapEntry(std::unordered_map<size_t, int>& anInputMap, std::string_view anActionType, int anInputCode)
	{
		const size_t actionHash = std::hash<std::string_view>{}(anActionType);

		if (!anInputMap.empty())
		{
			for (auto& [existingActionHash, inputCode] : anInputMap)
			{
				if (actionHash == existingActionHash)
				{
					inputCode = anInputCode;
					return;
				}
			}
		}

		anInputMap.emplace(actionHash, anInputCode);
	}

	void InputMapper::UpdateMouseAndKeyboard()
	{
		mySharedInputHandler->UpdateInput();

		for (const auto& [actionHash, keyCode] : myInputMap)
		{
			const bool isMouseButton = keyCode <= 0x06;

			if (mySharedInputHandler->IsKeyPressed(keyCode))
			{
				if (isMouseButton)
				{
					const CommonUtilities::Vector2<float> mousePosition = mySharedInputHandler->GetMousePosition().ToType<float>();
					const InputData data{ mousePosition.x, mousePosition.y, true, true, false };
					const InputEvent event{ true, false, true, data };
					DispatchEvent(actionHash, event);
				}
				else
				{
					const InputData data{ 1.0f, 1.0f, true, true, false };
					const InputEvent event{ true, false, false, data };
					DispatchEvent(actionHash, event);
				}
			}
			else if (mySharedInputHandler->IsKeyDown(keyCode))
			{
				if (isMouseButton)
				{
					const CommonUtilities::Vector2<float> mousePosition = mySharedInputHandler->GetMousePosition().ToType<float>();
					const InputData data{ mousePosition.x, mousePosition.y, true, false, false };
					const InputEvent event{ true, false, true, data };
					DispatchEvent(actionHash, event);
				}
				else
				{
					const InputData data{ 1.0f, 1.0f, true, false, false };
					const InputEvent event{ true, false, false, data };
					DispatchEvent(actionHash, event);
				}
			}
			else if (mySharedInputHandler->IsKeyReleased(keyCode))
			{
				if (isMouseButton)
				{
					const CommonUtilities::Vector2<float> mousePosition = mySharedInputHandler->GetMousePosition().ToType<float>();
					const InputData data{ mousePosition.x, mousePosition.y, false, false, true };
					const InputEvent event{ true, false, true, data };
					DispatchEvent(actionHash, event);
				}
				else
				{
					const InputData data{ 0.0f, 0.0f, false, false, true };
					const InputEvent event{ true, false, false, data };
					DispatchEvent(actionHash, event);
				}
			}
		}

		if (mySharedInputHandler->IsMouseMoved())
		{
			for (const auto& [actionHash, pointerCode] : myPointerMap)
			{
				InputData data{ 0.0f, 0.0f, false, false, false };

				if (pointerCode == static_cast<int>(EPointerCode::MOUSE_POSITION))
				{
					const CommonUtilities::Vector2<float> mousePosition = mySharedInputHandler->GetMousePosition().ToType<float>();
					data.valueA = mousePosition.x;
					data.valueB = mousePosition.y;
				}
				else if (pointerCode == static_cast<int>(EPointerCode::MOUSE_DELTA))
				{
					const CommonUtilities::Vector2<float> mouseDelta = mySharedInputHandler->GetMouseDelta().ToType<float>();
					data.valueA = mouseDelta.x;
					data.valueB = mouseDelta.y;
				}

				const InputEvent event{ false, false, true, data };
				DispatchEvent(actionHash, event);
			}
		}
	}

	void InputMapper::UpdateGamepad()
	{
		if (!mySharedXInputHandler->UpdateInput())
		{
			return;
		}

		for (auto it = myXInputMap.cbegin(); it != myXInputMap.cend(); ++it)
		{
			const int gamepadCode = it->second;

			switch (static_cast<EGamepadCode>(gamepadCode))
			{
			case EGamepadCode::TRIGGER_LEFT:
			{
				const float triggerValue = mySharedXInputHandler->GetTriggerLeftValue();
				const bool isNonZero = triggerValue > 0.0f;

				if (isNonZero || myPreviousLTriggerWasNonZero)
				{
					const size_t actionHash = it->first;
					const InputData data{ triggerValue, triggerValue, isNonZero, !myPreviousLTriggerWasNonZero, !isNonZero };
					const InputEvent event{ false, true, false, data };
					myPreviousLTriggerWasNonZero = isNonZero;

					DispatchEvent(actionHash, event);
				}

				break;
			}

			case EGamepadCode::TRIGGER_RIGHT:
			{
				const float triggerValue = mySharedXInputHandler->GetTriggerRightValue();
				const bool isNonZero = triggerValue > 0.0f;

				if (isNonZero || myPreviousRTriggerWasNonZero)
				{
					const size_t actionHash = it->first;
					const InputData data{ triggerValue, triggerValue, isNonZero, !myPreviousRTriggerWasNonZero, !isNonZero };
					const InputEvent event{ false, true, false, data };
					myPreviousRTriggerWasNonZero = isNonZero;

					DispatchEvent(actionHash, event);
				}

				break;
			}

			case EGamepadCode::ANALOG_LEFT:
			{
				CommonUtilities::Vector2<float> analogValue;
				const bool isNonZero = mySharedXInputHandler->GetAnalogLeftValue(analogValue);

				if (isNonZero || myPreviousLThumbWasNonZero)
				{
					const size_t actionHash = it->first;
					const InputData data{ analogValue.x, analogValue.y, isNonZero, !myPreviousLThumbWasNonZero, !isNonZero };
					const InputEvent event{ false, false, true, data };
					myPreviousLThumbWasNonZero = isNonZero;

					DispatchEvent(actionHash, event);
				}

				break;
			}

			case EGamepadCode::ANALOG_RIGHT:
			{
				CommonUtilities::Vector2<float> analogValue;
				const bool isNonZero = mySharedXInputHandler->GetAnalogRightValue(analogValue);

				if (isNonZero || myPreviousRThumbWasNonZero)
				{
					const size_t actionHash = it->first;
					const InputData data{ analogValue.x, analogValue.y, isNonZero, !myPreviousRThumbWasNonZero, !isNonZero };
					const InputEvent event{ false, false, true, data };
					myPreviousRThumbWasNonZero = isNonZero;

					DispatchEvent(actionHash, event);
				}

				break;
			}

			default:
			{
				if (mySharedXInputHandler->IsButtonPressed(gamepadCode))
				{
					const size_t actionHash = it->first;
					const InputData data{ 1.0f, 1.0f, true, true, false };
					const InputEvent event{ true, false, false, data };
					DispatchEvent(actionHash, event);
				}
				else if (mySharedXInputHandler->IsButtonDown(gamepadCode))
				{
					const size_t actionHash = it->first;
					const InputData data{ 1.0f, 1.0f, true, false, false };
					const InputEvent event{ true, false, false, data };
					DispatchEvent(actionHash, event);
				}
				else if (mySharedXInputHandler->IsButtonReleased(gamepadCode))
				{
					const size_t actionHash = it->first;
					const InputData data{ 0.0f, 0.0f, false, false, true };
					const InputEvent event{ true, false, false, data };
					DispatchEvent(actionHash, event);
				}
			}
			}
		}
	}

	void InputMapper::DispatchEvent(size_t anActionHash, const InputEvent& anInputEvent) const
	{
		auto listeners = myEventListeners.equal_range(anActionHash);

		for (auto& listener = listeners.first; listener != listeners.second; ++listener)
		{
			std::invoke(listener->second.second, anInputEvent);
		}
	}
}
