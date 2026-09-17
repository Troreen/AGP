#include "Actor.h"
#include "World.h"
#include "TransformOperations.h"

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

void Actor::SetName(std::string name)
{
    if (myWorld) myWorld->EnsureMutationAllowed();
    myName = std::move(name);
}
bool Actor::IsActive() const
{
	return myIsActive && !myPendingDestroy && (!GetParent() || GetParent()->IsActive());
}

void Actor::SetActive(bool active)
{
    if (myWorld) myWorld->EnsureMutationAllowed();
    myIsActive = active;
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
	myWorld = aWorld; myTransform.myWorld = aWorld;
}

bool Actor::CanAddComponentName(const std::string& aName) const
{
	return !aName.empty() && FindComponent(aName) == nullptr;
}

void Actor::ReportDuplicateComponentName(const std::string& name) const
{
    throw std::invalid_argument("Component names must be nonempty and unique: " + name);
}
std::string Actor::NextComponentName() const
{
    size_t number = myComponents.size() + myPendingComponents.size();
    std::string name;
    do { name = "Component " + std::to_string(++number); } while (FindComponent(name));
    return name;
}
bool Actor::CanAttach() const
{
    if (myWorld) myWorld->EnsureMutationAllowed();
    return myWorld && myWorld->AcceptsChanges() && !myPendingDestroy;
}
bool Actor::SetParent(Actor* parent, ReparentMode mode)
{
    if (!CanAttach() || (parent && (parent->GetWorld() != myWorld || parent->myPendingDestroy))) return false;
    if (parent && myAdmitted && !parent->myAdmitted) return false;
    for (auto* p = parent; p; p = p->GetParent()) if (p == this) return false;
    if (!GameFrameworkInternal::ChangeParent(myTransform.myValue, parent ? &parent->myTransform.myValue : nullptr, mode)) return false;
    myParent = parent ? parent->GetRef() : ActorRef{};
    return true;
}
