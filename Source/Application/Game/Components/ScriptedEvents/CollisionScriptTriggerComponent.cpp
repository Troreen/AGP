#include "CollisionScriptTriggerComponent.h"

#include "ScriptedEventComponent.h"
#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/Scenes/CustomComponentVariables.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"

#include <iostream>

void CollisionScriptTriggerComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	myEnteringActor = variables.GetOrDefault<std::string>("EnteringActor", "");
}

void CollisionScriptTriggerComponent::BeginPlay()
{
	auto triggerColliders = GetOwner()->GetComponents<ColliderComponent>();
	auto sequenceCoordinators = GetOwner()->GetComponents<ScriptedEventComponent>();
	if (triggerColliders.size() != 1 || sequenceCoordinators.size() != 1 || myEnteringActor.empty())
	{
		std::cerr << "Collision script trigger '" << GetOwner()->GetName()
			<< "' requires one collider, one sequence coordinator, and a nonempty EnteringActor\n";
		return;
	}
	myCollisionSubscription = triggerColliders.front()->OnEnter(this, &CollisionScriptTriggerComponent::OnEnter);
}

void CollisionScriptTriggerComponent::OnEnter(const Collision& collision)
{
	auto* triggerCollider = GetOwner()->GetComponent<ColliderComponent>();
	auto* sequenceCoordinator = GetOwner()->GetComponent<ScriptedEventComponent>();
	if (!triggerCollider || !triggerCollider->IsEnabled() || !sequenceCoordinator || !sequenceCoordinator->IsEnabled())
	{
		return;
	}

	ColliderComponent* enteringCollider = collision.OtherCollider();
	if (!enteringCollider || !enteringCollider->IsEnabled() || !enteringCollider->GetOwner()->IsActive() ||
	    enteringCollider->GetOwner()->GetName() != myEnteringActor)
	{
		return;
	}

	// If sequenceCoordinator->Play() returns true, that means the sequence started successfully.
	if (sequenceCoordinator->Play())
	{
		triggerCollider->SetEnabled(false);
		LOG(LogGameFramework, Log, "Collision scripted event '{}' started when Actor '{}' entered.",
			GetOwner()->GetName(), enteringCollider->GetOwner()->GetName());
	}
}

void CollisionScriptTriggerComponent::EndPlay() noexcept
{
	myCollisionSubscription.Reset();
}
