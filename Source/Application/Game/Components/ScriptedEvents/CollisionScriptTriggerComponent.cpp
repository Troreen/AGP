#include "CollisionScriptTriggerComponent.h"

#include "ScriptedEventComponent.h"
#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/EventBus.hpp"
#include "GameFramework/Scenes/CustomComponentVariables.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"

#include <iostream>

void CollisionScriptTriggerComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	myEnteringActor = variables.GetOrDefault<std::string>("EnteringActor", "");
}

void CollisionScriptTriggerComponent::BeginPlay()
{
	auto colliders = GetOwner()->GetComponents<ColliderComponent>();
	auto sequences = GetOwner()->GetComponents<ScriptedEventComponent>();
	if (colliders.size() != 1 || sequences.size() != 1 || myEnteringActor.empty())
	{
		std::cerr << "Collision script trigger '" << GetOwner()->GetName()
			<< "' requires one collider, one sequence coordinator, and a nonempty EnteringActor\n";
		return;
	}
	myColliderId = colliders.front()->GetColliderId();
	myListenerId = ServiceLocator::GetInstance().GetEventBus().AddEventListener<CollisionOverlapEvent>(
		[this](const CollisionOverlapEvent& event) { OnOverlap(event); });
	myListening = true;
}

void CollisionScriptTriggerComponent::OnOverlap(const CollisionOverlapEvent& event)
{
	if (event.Phase != CollisionOverlapPhase::Enter || !GetOwner() || !GetOwner()->IsActive() ||
		!IsEnabled())
	{
		return;
	}
	auto* collider = GetOwner()->GetComponent<ColliderComponent>();
	auto* sequence = GetOwner()->GetComponent<ScriptedEventComponent>();
	if (!collider || collider->GetColliderId() != myColliderId || !collider->IsEnabled() ||
		!sequence || !sequence->IsEnabled())
	{
		return;
	}
	if (event.FirstColliderId != myColliderId && event.SecondColliderId != myColliderId)
	{
		return;
	}
	const ColliderId otherId = event.FirstColliderId == myColliderId ? event.SecondColliderId : event.FirstColliderId;
	ColliderComponent* other = GetWorld().FindCollider(otherId);
	if (!other || !other->IsEnabled() || !other->GetOwner()->IsActive() ||
		other->GetOwner()->GetName() != myEnteringActor)
	{
		return;
	}
	if (sequence->Play())
	{
		collider->SetEnabled(false);
		LOG(LogGameFramework, Log, "Collision scripted event '{}' started when Actor '{}' entered.",
			GetOwner()->GetName(), other->GetOwner()->GetName());
	}
}

void CollisionScriptTriggerComponent::EndPlay() noexcept
{
	if (myListening)
	{
		ServiceLocator::GetInstance().GetEventBus().RemoveEventListener(myListenerId);
		myListening = false;
		myListenerId = 0;
	}
}
