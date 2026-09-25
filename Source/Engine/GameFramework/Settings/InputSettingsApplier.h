#pragma once

#include "EngineSettings.h"

namespace CommonUtilities
{
	class InputMapper;
}

// Applies settings to the input maps while keeping action listeners intact.
class InputSettingsApplier final
{
public:
	void Apply(CommonUtilities::InputMapper& anInputMapper, const InputSettings& someSettings);

private:
	InputSettings myAppliedSettings;
};
