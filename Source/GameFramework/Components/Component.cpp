#include "Component.h"

#include <utility>
#include "GameFramework/World/World.h"
#include <stdexcept>

void Component::FixedUpdate(float)
{
}

void Component::Update(float)
{
}

void Component::LateUpdate(float)
{
}

void Component::OnDestroy()
{
}

void Component::OnActiveChanged(bool)
{
}

void Component::OnEnabledChanged(bool)
{
}

const std::string& Component::GetName() const
{
	return myName;
}

Actor* Component::GetOwner() const
{
	return myOwner;
}

bool Component::IsEnabled() const
{
	return myIsEnabled && !myPendingDestroy;
}

void Component::SetEnabled(bool anIsEnabled)
{
	if (myIsEnabled == anIsEnabled)
	{
		return;
	}

	myIsEnabled = anIsEnabled;
	OnEnabledChanged(myIsEnabled);
}

void Component::SetOwner(Actor* anOwner)
{
	myOwner = anOwner;
}

void Component::SetName(std::string aName)
{
	myName = std::move(aName);
}

World& Component::GetWorld() const
{
    if (!myOwner || !myOwner->GetWorld()) throw std::logic_error("Component is not attached");
    return *myOwner->GetWorld();
}
const GameInput& Component::GetInput() const { return GetWorld().GetInput(); }
void Component::Destroy() { GetWorld().DestroyComponent(*this); }
