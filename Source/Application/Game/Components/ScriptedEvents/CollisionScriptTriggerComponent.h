#pragma once

#include "GameFramework/Components/CustomComponent.h"
#include "GameFramework/Collision/CollisionOverlapEvent.h"

#include <string>

// Starts the sequence on this Actor when the configured Actor enters its collider.
class CollisionScriptTriggerComponent final : public CustomComponent
{
public:
	void InitializeCustomVariables(const CustomComponentVariables& variables) override;
	void BeginPlay() override;
	void EndPlay() noexcept override;

private:
	void OnOverlap(const CollisionOverlapEvent& event);

	std::string myEnteringActor;
	ColliderId myColliderId = 0;
	unsigned myListenerId = 0;
	bool myListening = false;
};
