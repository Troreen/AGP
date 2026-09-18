#include "GameFramework/World/Component.h"
#include "GameFramework/World/World.h"
#include <stdexcept>

World& Component::GetWorld() const
{
	if (!myOwner)
	{
		throw std::logic_error("Component is not attached to an Actor");
	}
	return *myOwner->GetWorld();
}

InputSystem& Component::GetInputSystem() const
{
	return GetWorld().GetInputSystem();
}
