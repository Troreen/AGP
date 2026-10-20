#include "Components/CollisionDemoComponent.h"

#include "GameLog.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/World/World.h"
#include "Maths.hpp"

#include <algorithm>
#include <array>


CollisionDemoComponent::CollisionDemoComponent(ColliderId aMovingSphere, ColliderId aStationarySphere,
	ColliderId aStationaryBox, ColliderId aFilteredSphere)
	: myMovingSphereId(aMovingSphere), myStationarySphereId(aStationarySphere),
	myStationaryBoxId(aStationaryBox), myFilteredSphereId(aFilteredSphere)
{
}

void CollisionDemoComponent::BeginPlay()
{
	// Camera selection is finished before BeginPlay. Place the sample in front of
	// that view once; moving the camera later does not move the volumes with it.
	if (const CameraComponent* camera = GetWorld().GetActiveCamera())
	{
		const CU::Vector3f forward = camera->GetWorldDirection();
		myCenter = camera->GetWorldPosition() + forward * 650.0f;
		myRight = CU::NormalizeSafe(CU::Vector3f::UnitY.Cross(forward), CU::Vector3f::UnitX);
	}
	const std::array<ColliderId, 4> ids{myMovingSphereId, myStationarySphereId, myStationaryBoxId, myFilteredSphereId};
	const std::array<float, 4> offsets{-320, -120, 200, 80};
	for (size_t index = 0; index < ids.size(); ++index)
	{
		if (ColliderComponent* collider = GetWorld().FindCollider(ids[index]))
		{
			collider->GetOwner()->GetTransform().SetWorldPosition(myCenter + myRight * offsets[index]);
			collider->SetDebugDrawEnabled(true);
			collider->SetDebugColor(ids[index] == myMovingSphereId ? CU::Vector4f{0, 1, 1, 1} :
				ids[index] == myFilteredSphereId ? CU::Vector4f{1, 0, 1, 1} : CU::Vector4f{1, 1, 0, 1});
		}
	}
	if (auto* moving = GetWorld().FindCollider(myMovingSphereId))
	{
		myEnterSubscription = moving->OnEnter(this, &CollisionDemoComponent::OnCollision);
		myStaySubscription = moving->OnStay(this, &CollisionDemoComponent::OnCollision);
		myExitSubscription = moving->OnExit(this, &CollisionDemoComponent::OnCollision);
	}
	GAMELOG(Log, "Collision demo: cyan moving sphere; yellow fixed sphere/box; magenta filtered sphere. F4 restarts.");
	GAMELOG(Log, "Collision demo IDs: moving={}, fixed={}, box={}, filtered={} (filtered ID must never produce an event).",
		myMovingSphereId, myStationarySphereId, myStationaryBoxId, myFilteredSphereId);
}

void CollisionDemoComponent::Update(float aDeltaTime)
{
	const float previousTime = myElapsedTime;
	myElapsedTime += (std::max)(aDeltaTime, 0.0f);
	if (ColliderComponent* moving = GetWorld().FindCollider(myMovingSphereId))
	{
		float offset = -120.0f;
		if (myElapsedTime < 4.0f)
		{
			offset = -320.0f + myElapsedTime * 50.0f;
		}
		else if (myElapsedTime >= 7.0f && myElapsedTime < 15.0f)
		{
			offset = -120.0f + (myElapsedTime - 7.0f) * 40.0f;
		}
		else if (myElapsedTime >= 15.0f)
		{
			offset = 200.0f + std::clamp(myElapsedTime - 17.0f, 0.0f, 3.0f) * 60.0f;
		}
		moving->GetOwner()->GetTransform().SetWorldPosition(myCenter + myRight * offset);
		moving->SetEnabled(myElapsedTime < 5.0f || myElapsedTime >= 6.0f);
	}
	if (previousTime < 5.0f && myElapsedTime >= 5.0f)
	{
		GAMELOG(Log, "Collision demo 5s: disable moving sphere while overlapping; expect Exit and gray outline.");
	}
	if (previousTime < 6.0f && myElapsedTime >= 6.0f)
	{
		GAMELOG(Log, "Collision demo 6s: re-enable moving sphere; expect Enter then Stay.");
	}
	if (previousTime < 9.75f && myElapsedTime >= 9.75f)
	{
		GAMELOG(Log, "Collision demo: crossing magenta layer-2 sphere; moving mask accepts only layer 1, so no event for its ID.");
	}
	if (previousTime < 16.0f && myElapsedTime >= 16.0f)
	{
		if (ColliderComponent* box = GetWorld().FindCollider(myStationaryBoxId))
		{
			box->GetOwner()->Destroy();
		}
		GAMELOG(Log, "Collision demo 16s: destroy overlapping box; expect its outline to disappear and Exit (lookup may be null).");
	}
	if (previousTime < 20.0f && myElapsedTime >= 20.0f)
	{
		GAMELOG(Log, "Collision demo complete. F4 reloads the scene, removes this listener, and starts a new run.");
	}

}

void CollisionDemoComponent::OnCollision(const Collision& collision)
{
	const ColliderId otherColliderId = collision.OtherColliderId;
	const char* phaseName = "Enter";
	if (collision.Phase == CollisionOverlapPhase::Stay)
	{
		// One Stay log per entry keeps the console readable; delivery still happens every frame.
		if (!myReportedStayIds.insert(otherColliderId).second)
		{
			return;
		}
		phaseName = "Stay";
	}
	else if (collision.Phase == CollisionOverlapPhase::Exit)
	{
		myReportedStayIds.erase(otherColliderId);
		phaseName = "Exit";
	}
	GAMELOG(Log, "Collision demo {:.2f}s: {} moving={} other={} (other alive={}).", myElapsedTime,
		phaseName, myMovingSphereId, otherColliderId, GetWorld().FindCollider(otherColliderId) != nullptr);
}

void CollisionDemoComponent::EndPlay() noexcept
{
	myEnterSubscription.Reset();
	myStaySubscription.Reset();
	myExitSubscription.Reset();
	GAMELOG(Log, "Collision demo EndPlay: listener removed.");
}
