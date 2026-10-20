#pragma once
#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/World/Component.h"
#include "Vector3.hpp"

#include <cstdint>
#include <unordered_set>

// Optional, one-shot shape demonstration. The World owns every volume Actor.
class CollisionDemoComponent final : public Component
{
public:
	CollisionDemoComponent(ColliderId aMovingSphere, ColliderId aStationarySphere,
		ColliderId aStationaryBox, ColliderId aFilteredSphere);
	void BeginPlay() override;
	void Update(float aDeltaTime) override;
	void EndPlay() noexcept override;

private:
	void OnCollision(const Collision& collision);
	ColliderId myMovingSphereId;
	ColliderId myStationarySphereId;
	ColliderId myStationaryBoxId;
	ColliderId myFilteredSphereId;
	CollisionSubscription myEnterSubscription;
	CollisionSubscription myStaySubscription;
	CollisionSubscription myExitSubscription;
	CommonUtilities::Vector3f myCenter{};
	CommonUtilities::Vector3f myRight{1, 0, 0};
	float myElapsedTime = 0;
	std::unordered_set<ColliderId> myReportedStayIds;
};
