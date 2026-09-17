#pragma once
#include "EnumKeyCode.h"
#include "EnumGamepadCode.h"
#include "EnumPointerCode.h"
#include <vector>
#include <string>
#include <string_view>
#include <utility>
#include <functional>
#include <unordered_map>

namespace CommonUtilities
{
	class InputHandler;
	class XInputHandler;

	struct InputData
	{
		float valueA;
		float valueB;
		bool isHeld;
		bool isPressed;
		bool isReleased;
	};

	struct InputEvent
	{
		bool isButton;
		bool isAxis1D;
		bool isAxis2D;
		const InputData& inputData;
	};

	class InputMapper
	{
		public:
			InputMapper();
			~InputMapper() = default;

			void Init(CommonUtilities::InputHandler* anInputHandler, CommonUtilities::XInputHandler* anXInputHandler = nullptr);

			void BindActionToInputCode(std::string_view anActionType, EKeyCode aKeyCode);
			void BindActionToInputCode(std::string_view anActionType, EPointerCode aPointerCode);
			void BindActionToInputCode(std::string_view anActionType, EGamepadCode aGamepadCode);
			void ClearBindingsFromAction(std::string_view anActionType);
			void Update();

			const CommonUtilities::InputHandler* GetInputHandler() const;
			CommonUtilities::InputHandler* GetInputHandler();

			const CommonUtilities::XInputHandler* GetXInputHandler() const;
			CommonUtilities::XInputHandler* GetXInputHandler();

			unsigned AddEventListener(std::string_view anActionType, std::function<void(const InputEvent&)> aCallback);
			void RemoveEventListener(unsigned anEventListenerID);

		private:
			std::unordered_multimap<size_t, std::pair<unsigned, std::function<void(const InputEvent&)>>> myEventListeners;
			std::unordered_map<size_t, int> myInputMap;
			std::unordered_map<size_t, int> myXInputMap;
			std::unordered_map<size_t, int> myPointerMap;

			CommonUtilities::InputHandler* mySharedInputHandler;
			CommonUtilities::XInputHandler* mySharedXInputHandler;

			unsigned myEventListenerCreationCount;

			bool myPreviousLTriggerWasNonZero;
			bool myPreviousRTriggerWasNonZero;
			bool myPreviousLThumbWasNonZero;
			bool myPreviousRThumbWasNonZero;

			void UpdateInputMapEntry(std::unordered_map<size_t, int>& anInputMap, std::string_view anActionType, int anInputCode);
			void UpdateMouseAndKeyboard();
			void UpdateGamepad();
			void DispatchEvent(size_t anActionHash, const InputEvent& anInputEvent) const;
	};
}
