#pragma once
#include "Components/CustomComponentRegistry.h"
#include "Components/SpinComponent.h"

inline void RegisterCustomComponents(CustomComponentRegistry& aRegistry)
{
	aRegistry.Register<SpinComponent>("SpinComponent");
}
