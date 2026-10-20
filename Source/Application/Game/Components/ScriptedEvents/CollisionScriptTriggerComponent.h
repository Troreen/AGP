#pragma once

#include "GameFramework/Components/CustomComponent.h"
#include "GameFramework/Components/ColliderComponent.h"

#include <string>

// Starts the sequence on this Actor when the configured Actor enters its collider.
class CollisionScriptTriggerComponent final : public CustomComponent
{
public:
	void InitializeCustomVariables(const CustomComponentVariables& variables) override;
	void BeginPlay() override;
	void EndPlay() noexcept override;

private:
	void OnEnter(const Collision& collision);

	std::string myEnteringActor;
	CollisionSubscription myCollisionSubscription;
};
