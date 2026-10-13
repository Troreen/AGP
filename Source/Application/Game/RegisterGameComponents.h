#pragma once
#include "GameComponentRegistry.h"
#include "SpinComponent.h"

inline void RegisterGameComponents(GameComponentRegistry& aRegistry)
{
	aRegistry.Register<SpinComponent>("SpinComponent");
}
