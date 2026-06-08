#include "Component.h"

#include <utility>

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
	return myIsEnabled;
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
