#include "Actor.h"
#include "World.h"

#include "GameFramework/Diagnostics/GameFrameworkLog.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>
#include <stdexcept>

Actor::Actor(std::string aName)
	: myName(std::move(aName))
{
}

Actor::~Actor()
{
	// World performs lifecycle cleanup before releasing ownership.
}

void Actor::FixedUpdate(float aDeltaTime)
{
	if (!IsActive())
	{
		return;
	}

	for (std::unique_ptr<Component>& component : myComponents)
	{
		if (IsActive() && component->myBegun && component->IsEnabled())
		{
			component->FixedUpdate(aDeltaTime);
		}
	}
}

void Actor::Update(float aDeltaTime)
{
	if (!IsActive())
	{
		return;
	}

	for (std::unique_ptr<Component>& component : myComponents)
	{
		if (IsActive() && component->myBegun && component->IsEnabled())
		{
			component->Update(aDeltaTime);
		}
	}
}

void Actor::LateUpdate(float aDeltaTime)
{
	if (!IsActive())
	{
		return;
	}

	for (std::unique_ptr<Component>& component : myComponents)
	{
		if (IsActive() && component->myBegun && component->IsEnabled())
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
	if (aName.empty() || (myWorld && myWorld->FindActor(aName) && myWorld->FindActor(aName) != this))
        throw std::invalid_argument("Actor name must remain nonempty and unique");
    myName = std::move(aName);
}

bool Actor::IsActive() const
{
	return myIsActive && !myPendingDestroy && (!GetParent() || GetParent()->IsActive());
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
		if (!component->IsPendingDestroy() && component->GetName() == aName)
		{
			return component.get();
		}
	}

    for (const auto& component : myPendingComponents)
        if (!component->IsPendingDestroy() && component->GetName() == aName) return component.get();
	return nullptr;
}

void Actor::RemoveAllComponents()
{
    for (auto& c : myComponents) c->Destroy();
    for (auto& c : myPendingComponents) c->Destroy();
}
void Actor::Destroy() { if (myWorld) myWorld->DestroyActor(*this); }
void Actor::AttachComponent(std::unique_ptr<Component> component)
{
    myWorld->Attach(*this, std::move(component));
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

bool Actor::CanAttach() const { return myWorld && myWorld->AcceptsChanges() && !myPendingDestroy; }

bool Actor::ApplyParent(Actor* parent, ReparentMode mode)
{
    if (myPendingDestroy || (parent && (parent->GetWorld() != myWorld || parent->myPendingDestroy))) return false;
    for (auto* p = parent; p; p = p->GetParent()) if (p == this) return false;
    if (!GameFrameworkInternal::ChangeParent(myTransform, parent ? &parent->myTransform : nullptr, mode)) return false;
    myParent = parent ? parent->GetHandle() : ActorHandle{}; return true;
}
bool Actor::SetParent(Actor* parent, ReparentMode mode)
{
    if (!CanAttach() || (parent && (parent->GetWorld() != myWorld || parent->myPendingDestroy))) return false;
    for (auto* p = parent; p; p = p->GetParent()) if (p == this) return false;
    if (myWorld->GetState() != World::State::Active) return ApplyParent(parent, mode);
    auto self = GetHandle(); auto target = parent ? parent->GetHandle() : ActorHandle{};
    myWorld->QueueStructure([self,target,hasParent = parent != nullptr,mode](SceneDiagnostics& errors)
    {
        auto* object = self.Get(); if (!object) return;
        if ((hasParent && !target.Get()) || !object->ApplyParent(target.Get(),mode))
            errors.push_back({object->GetName(),{},"parent","Invalid actor attachment"});
    });
    return true;
}
