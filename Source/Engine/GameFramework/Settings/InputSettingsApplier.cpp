#include "InputSettingsApplier.h"

#include "InputMapper.h"

#include <set>
#include <string>

void InputSettingsApplier::Apply(CommonUtilities::InputMapper& anInputMapper, const InputSettings& someSettings)
{
	std::set<std::string> actionNames;
	for (const auto& action : myAppliedSettings.Actions)
	{
		actionNames.insert(action.first);
	}
	for (const auto& action : someSettings.Actions)
	{
		actionNames.insert(action.first);
	}

	for (const std::string& actionName : actionNames)
	{
		const auto previousAction = myAppliedSettings.Actions.find(actionName);
		const auto requestedAction = someSettings.Actions.find(actionName);
		const ActionBindings previousBindings = previousAction == myAppliedSettings.Actions.end() ? ActionBindings{} : previousAction->second;
		const ActionBindings requestedBindings = requestedAction == someSettings.Actions.end() ? ActionBindings{} : requestedAction->second;

		if (previousBindings.Key != requestedBindings.Key)
		{
			anInputMapper.RemoveKeyBindingFromAction(actionName);
			if (requestedBindings.Key)
			{
				anInputMapper.BindActionToInputCode(actionName, *requestedBindings.Key);
			}
		}

		if (previousBindings.Pointer != requestedBindings.Pointer)
		{
			anInputMapper.RemovePointerBindingFromAction(actionName);
			if (requestedBindings.Pointer)
			{
				anInputMapper.BindActionToInputCode(actionName, *requestedBindings.Pointer);
			}
		}

		if (previousBindings.Gamepad != requestedBindings.Gamepad)
		{
			anInputMapper.RemoveGamepadBindingFromAction(actionName);
			if (requestedBindings.Gamepad)
			{
				anInputMapper.BindActionToInputCode(actionName, *requestedBindings.Gamepad);
			}
		}
	}

	myAppliedSettings = someSettings;
}
