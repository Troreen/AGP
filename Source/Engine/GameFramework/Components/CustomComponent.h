#pragma once
#include "GameFramework/World/Component.h"

struct CustomComponentVariables;

class CustomComponent : public Component
{
public:
	// Called after common component data is applied, before BeginPlay.
	// Resolve references to other components in BeginPlay.
	virtual void ApplyCustomVariables(const CustomComponentVariables& someVariables) = 0;
};
