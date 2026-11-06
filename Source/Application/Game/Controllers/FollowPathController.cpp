#include "FollowPathController.h"
#include <GameFramework/World/Actor.h>
#include "ControllerComponent.h"
#include <GameFramework/ServiceLocator.h>
#include <AISystems/Pathfinder.hpp>

void FollowPathController::UpdateActorControllerData(ComponentControllerData& outData)
{
	float& minPathDistance = outData.MinPathDistance;
	float& maxPlayerDistanceFromPath = outData.MaxPlayerDistanceFromPath;
	int& pathIndex = outData.PathIndex;

	std::vector<CommonUtilities::Vector3f>& path = outData.Path;
	CommonUtilities::Vector3f ownerPos = outData.OwnerPos;
	CommonUtilities::Vector3f playerPos = ServiceLocator::GetInstance().GetPlayer()->GetTransform().GetWorldPosition();

	if (pathIndex == -1 || (playerPos - path[0]).LengthSqr() > maxPlayerDistanceFromPath * maxPlayerDistanceFromPath)
	{
		ServiceLocator::GetInstance().GetPathFinder().ImmediateRequest(ownerPos, playerPos, path);
		pathIndex = static_cast<int>(path.size()) - 1;
	}

	if ((ownerPos - path[pathIndex]).LengthSqr() < minPathDistance)
	{
		pathIndex--;
	}
}

const CommonUtilities::Vector3f FollowPathController::GetDesiredVelocity(ControllerComponent* aComponent) const
{
	ComponentControllerData& data = aComponent->GetData();

	int& pathIndex = data.PathIndex;
	float speed = data.Speed;

	std::vector<CommonUtilities::Vector3f>& path = data.Path;
	CommonUtilities::Vector3f ownerPos = data.OwnerPos;

	CommonUtilities::Vector3f direction = (path[pathIndex] - ownerPos).GetNormalized();

	return direction * speed;
}

const CommonUtilities::Vector3f FollowPathController::GetSteeringForce(ControllerComponent* aComponent) const
{
	CommonUtilities::Vector3f desired = GetDesiredVelocity(aComponent);
	CommonUtilities::Vector3f velocity = aComponent->GetVelocity();
	float steering = aComponent->GetData().Steering;

	return (desired - velocity).GetNormalized() * steering;
}
