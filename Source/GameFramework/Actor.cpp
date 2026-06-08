#include "Actor.h"

#include "GameFrameworkLog.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

Actor::Actor(std::string aName)
	: myName(std::move(aName))
{
}

Actor::~Actor()
{
	RemoveAllComponents();
}

void Actor::Update(float aDeltaTime)
{
	if (!myIsActive)
	{
		return;
	}

	for (std::unique_ptr<Component>& component : myComponents)
	{
		if (component->IsEnabled())
		{
			component->Update(aDeltaTime);
		}
	}
}

void Actor::LateUpdate(float aDeltaTime)
{
	if (!myIsActive)
	{
		return;
	}

	for (std::unique_ptr<Component>& component : myComponents)
	{
		if (component->IsEnabled())
		{
			component->LateUpdate(aDeltaTime);
		}
	}
}

const std::string& Actor::GetName() const
{
	return myName;
}

void Actor::SetName(std::string aName)
{
	myName = std::move(aName);
}

bool Actor::IsActive() const
{
	return myIsActive;
}

void Actor::SetActive(bool anIsActive)
{
	if (myIsActive == anIsActive)
	{
		return;
	}

	myIsActive = anIsActive;

	for (std::unique_ptr<Component>& component : myComponents)
	{
		component->OnActiveChanged(myIsActive);
	}
}

CommonUtilities::Transform& Actor::GetTransform()
{
	return myTransform;
}

const CommonUtilities::Transform& Actor::GetTransform() const
{
	return myTransform;
}

void Actor::SetTranslation(const CommonUtilities::Vector3<float>& aTranslation)
{
	myTransform.SetPosition(aTranslation);
}

void Actor::SetPosition(const CommonUtilities::Vector3<float>& aPosition)
{
	myTransform.SetPosition(aPosition);
}

void Actor::SetRotation(const CommonUtilities::Quaternion<float>& aRotation)
{
	myTransform.SetRotation(aRotation);
}

void Actor::SetRotation(float aYawDegrees, float aPitchDegrees, float aRollDegrees)
{
	myTransform.SetRotation(aYawDegrees, aPitchDegrees, aRollDegrees);
}

void Actor::SetScale(const CommonUtilities::Vector3<float>& aScale)
{
	myTransform.SetScale(aScale);
}

void Actor::LookAt(const CommonUtilities::Vector3<float>& aTarget)
{
	const CommonUtilities::Vector3<float> position = myTransform.GetPosition();
	CommonUtilities::Vector3<float> forward = (aTarget - position).GetNormalized();
	if (forward.LengthSqr() == 0.0f)
	{
		return;
	}

	const float yaw = std::atan2(forward.x, forward.z);
	const float pitch = -std::asin(std::clamp(forward.y, -1.0f, 1.0f));
	myTransform.SetYawPitchRollRadians(yaw, pitch, 0.0f);
}

World* Actor::GetWorld() const
{
	return myWorld;
}

Component* Actor::FindComponent(const std::string& aName) const
{
	for (const std::unique_ptr<Component>& component : myComponents)
	{
		if (component->GetName() == aName)
		{
			return component.get();
		}
	}

	return nullptr;
}

void Actor::RemoveAllComponents()
{
	for (auto it = myComponents.rbegin(); it != myComponents.rend(); ++it)
	{
		(*it)->OnDestroy();
	}

	myComponents.clear();
}

void Actor::SetWorld(World* aWorld)
{
	myWorld = aWorld;
}

bool Actor::CanAddComponentName(const std::string& aName) const
{
	return !aName.empty() && FindComponent(aName) == nullptr;
}

void Actor::ReportDuplicateComponentName(const std::string& aName) const
{
	GFLOG(Error, "Actor '{}' could not add component '{}'. Component names must be non-empty and unique per actor.", myName, aName);
	assert(false && "Duplicate or empty component name");
}
