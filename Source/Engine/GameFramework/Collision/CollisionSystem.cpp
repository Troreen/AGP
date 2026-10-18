#include "GameFramework/Collision/CollisionSystem.h"
#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/EventBus.hpp"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/GameFrameworkLog.h"
#include <algorithm>
#include <cmath>

namespace
{
	using CollisionVector = CommonUtilities::Vector3<double>;
	constexpr double TransformTolerance = 0.00001;

	// Allow a little rounding error so shapes that just touch still count as overlapping.
	constexpr double TouchingTolerance = 1.000001;

	struct WorldColliderShape
	{
		ColliderId Id;
		unsigned int Layer;
		std::uint32_t Mask;
		CollisionVector Center;
		double Radius;
		ColliderShape Type;
		CollisionVector HalfExtents;
		CollisionVector Right;
		CollisionVector Forward;
	};

	bool ReadTransformAxesAndScale(const CommonUtilities::Matrix4f& transformMatrix, CollisionVector (&normalizedAxes)[3],
	                               CollisionVector& axisScales)
	{
		// Reject broken matrices before using them in the collision calculations.
		for (int row = 1; row <= 4; ++row)
		{
			for (int column = 1; column <= 4; ++column)
			{
				if (!std::isfinite(transformMatrix(row, column)))
				{
					return false;
				}
			}
		}

		// Collision shapes need a regular position, rotation and scale transform.
		if (std::abs(transformMatrix(1, 4)) > TransformTolerance || std::abs(transformMatrix(2, 4)) > TransformTolerance ||
		    std::abs(transformMatrix(3, 4)) > TransformTolerance || std::abs(transformMatrix(4, 4) - 1) > TransformTolerance)
		{
			return false;
		}

		// Each row gives a direction and its scale. Remove the scale to get unit directions.
		normalizedAxes[0] = {transformMatrix(1, 1), transformMatrix(1, 2), transformMatrix(1, 3)};
		normalizedAxes[1] = {transformMatrix(2, 1), transformMatrix(2, 2), transformMatrix(2, 3)};
		normalizedAxes[2] = {transformMatrix(3, 1), transformMatrix(3, 2), transformMatrix(3, 3)};
		axisScales = {normalizedAxes[0].Length(), normalizedAxes[1].Length(), normalizedAxes[2].Length()};

		if (axisScales.x <= 0 || axisScales.y <= 0 || axisScales.z <= 0)
		{
			return false;
		}

		normalizedAxes[0] /= axisScales.x;
		normalizedAxes[1] /= axisScales.y;
		normalizedAxes[2] /= axisScales.z;

		// The directions must be perpendicular and must not describe a mirrored shape.
		if (std::abs(normalizedAxes[0].Dot(normalizedAxes[1])) > TransformTolerance ||
		    std::abs(normalizedAxes[0].Dot(normalizedAxes[2])) > TransformTolerance ||
		    std::abs(normalizedAxes[1].Dot(normalizedAxes[2])) > TransformTolerance ||
		    normalizedAxes[0].Cross(normalizedAxes[1]).Dot(normalizedAxes[2]) < 1 - TransformTolerance)
		{
			return false;
		}

		return true;
	}

	bool ReadWorldColliderShape(const ColliderComponent& collider, WorldColliderShape& worldShape)
	{
		CollisionVector normalizedAxes[3];
		CollisionVector axisScales;
		const bool isBox = collider.GetShape() == ColliderShape::Box;

		// Boxes may turn left or right, but they must stay upright.
		const auto hasOnlyYawRotation = [&]()
		{
			return std::abs(normalizedAxes[1].y - 1) <= TransformTolerance && std::abs(normalizedAxes[0].y) <= TransformTolerance &&
			       std::abs(normalizedAxes[2].y) <= TransformTolerance;
		};

		// Check the collider and its owner separately before combining them.
		// Otherwise, two unsupported transforms could cancel out and hide the problem.
		if (!ReadTransformAxesAndScale(collider.GetTransform().GetLocalMatrix(), normalizedAxes, axisScales) ||
		    (isBox && !hasOnlyYawRotation()) ||
		    !ReadTransformAxesAndScale(collider.GetOwner()->GetTransform().GetWorldMatrix(), normalizedAxes, axisScales) ||
		    (isBox && !hasOnlyYawRotation()))
		{
			return false;
		}

		const auto transformMatrix = collider.GetWorldMatrix();
		if (!ReadTransformAxesAndScale(transformMatrix, normalizedAxes, axisScales) || (isBox && !hasOnlyYawRotation()))
		{
			return false;
		}

		// A sphere needs the same scale in every direction to remain a sphere.
		if (!isBox && (std::abs(axisScales.x - axisScales.y) > TransformTolerance * axisScales.x ||
		               std::abs(axisScales.x - axisScales.z) > TransformTolerance * axisScales.x))
		{
			return false;
		}

		// Store the world-space values once so every pair uses the same snapshot.
		const auto& localBoxHalfExtents = collider.GetBoxHalfExtents();
		worldShape = {collider.GetColliderId(),
		              collider.GetLayer(),
		              collider.GetMask(),
		              {transformMatrix(4, 1), transformMatrix(4, 2), transformMatrix(4, 3)},
		              collider.GetSphereRadius() * axisScales.x,
		              collider.GetShape(),
		              {localBoxHalfExtents.x * axisScales.x, localBoxHalfExtents.y * axisScales.y, localBoxHalfExtents.z * axisScales.z},
		              normalizedAxes[0],
		              normalizedAxes[2]};

		return std::isfinite(worldShape.Radius);
	}

	bool SphereOverlapsBox(const WorldColliderShape& sphereShape, const WorldColliderShape& boxShape)
	{
		// Find how far the sphere center sits outside the box in each direction.
		// Inside the box, that distance is zero. The remaining gap must fit within the sphere radius.
		const CollisionVector centerOffset = sphereShape.Center - boxShape.Center;
		const double distanceOutsideRightFace = (std::max)(0.0, std::abs(centerOffset.Dot(boxShape.Right)) - boxShape.HalfExtents.x);
		const double distanceOutsideTopFace = (std::max)(0.0, std::abs(centerOffset.y) - boxShape.HalfExtents.y);
		const double distanceOutsideForwardFace = (std::max)(0.0, std::abs(centerOffset.Dot(boxShape.Forward)) - boxShape.HalfExtents.z);

		const double allowedSphereRadius = sphereShape.Radius * TouchingTolerance;

		return distanceOutsideRightFace * distanceOutsideRightFace + distanceOutsideTopFace * distanceOutsideTopFace +
		           distanceOutsideForwardFace * distanceOutsideForwardFace <=
		       allowedSphereRadius * allowedSphereRadius;
	}

	double GetBoxRadiusAlongAxis(const WorldColliderShape& worldShape, const CollisionVector& projectionAxis)
	{
		// Measure how far the box reaches from its center along the direction being checked.
		return worldShape.HalfExtents.x * std::abs(worldShape.Right.Dot(projectionAxis)) +
		       worldShape.HalfExtents.z * std::abs(worldShape.Forward.Dot(projectionAxis));
	}

	bool BoxesOverlap(const WorldColliderShape& firstShape, const WorldColliderShape& secondShape)
	{
		// Upright boxes cannot overlap if there is a gap between their heights.
		const CollisionVector centerOffset = secondShape.Center - firstShape.Center;
		if (std::abs(centerOffset.y) > (firstShape.HalfExtents.y + secondShape.HalfExtents.y) * TouchingTolerance)
		{
			return false;
		}

		// Now look for a gap along either box's horizontal directions.
		// A gap in any one direction is enough to rule out an overlap.
		const CollisionVector horizontalTestAxes[] = {firstShape.Right, firstShape.Forward, secondShape.Right, secondShape.Forward};
		for (const CollisionVector& projectionAxis : horizontalTestAxes)
		{
			if (std::abs(centerOffset.Dot(projectionAxis)) >
			    (GetBoxRadiusAlongAxis(firstShape, projectionAxis) + GetBoxRadiusAlongAxis(secondShape, projectionAxis)) *
			        TouchingTolerance)
			{
				return false;
			}
		}

		return true;
	}

	bool Overlaps(const WorldColliderShape& firstShape, const WorldColliderShape& secondShape)
	{
		if (firstShape.Type == ColliderShape::Box && secondShape.Type == ColliderShape::Box)
		{
			return BoxesOverlap(firstShape, secondShape);
		}

		if (firstShape.Type == ColliderShape::Box)
		{
			return SphereOverlapsBox(secondShape, firstShape);
		}

		if (secondShape.Type == ColliderShape::Box)
		{
			return SphereOverlapsBox(firstShape, secondShape);
		}

		// Both shapes are spheres here. Compare their center distance with their combined radii.
		const double combinedSphereRadius = (firstShape.Radius + secondShape.Radius) * TouchingTolerance;
		return (firstShape.Center - secondShape.Center).LengthSqr() <= combinedSphereRadius * combinedSphereRadius;
	}
}

void CollisionSystem::Update(World& world, const std::vector<Component*>& frameComponents)
{
	// Collect the colliders that can participate this frame before checking any pairs.
	std::vector<WorldColliderShape> worldColliderShapes;
	for (Component* component : frameComponents)
	{
		if (!component->HasBegunPlay() || !component->IsEnabled() || !component->GetOwner()->IsActive())
		{
			continue;
		}

		if (auto* collider = dynamic_cast<ColliderComponent*>(component))
		{
			WorldColliderShape worldShape{};
			if (ReadWorldColliderShape(*collider, worldShape))
			{
				worldColliderShapes.push_back(worldShape);
			}
			else
			{
				GFLOG(Warning, "Collider {} excluded: unsupported or nonfinite transform", collider->GetColliderId());
			}
		}
	}

	// Check each pair once. Both colliders must allow the other's layer.
	std::set<std::pair<ColliderId, ColliderId>> currentOverlapPairs;
	std::vector<CollisionOverlapEvent> pendingOverlapEvents;
	for (size_t firstShapeIndex = 0; firstShapeIndex < worldColliderShapes.size(); ++firstShapeIndex)
	{
		for (size_t secondShapeIndex = firstShapeIndex + 1; secondShapeIndex < worldColliderShapes.size(); ++secondShapeIndex)
		{
			const WorldColliderShape& firstShape = worldColliderShapes[firstShapeIndex];
			const WorldColliderShape& secondShape = worldColliderShapes[secondShapeIndex];
			if (!(firstShape.Mask & (std::uint32_t{1} << secondShape.Layer)) ||
			    !(secondShape.Mask & (std::uint32_t{1} << firstShape.Layer)) || !Overlaps(firstShape, secondShape))
			{
				continue;
			}

			// Keep the IDs in a fixed order so we can recognize the same pair next frame.
			const auto overlapPair = std::minmax(firstShape.Id, secondShape.Id);
			currentOverlapPairs.emplace(overlapPair.first, overlapPair.second);

			// An overlap already seen last frame continues with Stay; a new one starts with Enter.
			pendingOverlapEvents.push_back({&world, overlapPair.first, overlapPair.second,
			                                myPairs.contains(overlapPair) ? CollisionOverlapPhase::Stay : CollisionOverlapPhase::Enter});
		}
	}

	// Pairs that were touching last frame but are missing now need an Exit event.
	for (const auto& overlapPair : myPairs)
	{
		if (!currentOverlapPairs.contains(overlapPair))
		{
			pendingOverlapEvents.push_back({&world, overlapPair.first, overlapPair.second, CollisionOverlapPhase::Exit});
		}
	}

	// Save this frame's overlaps before notifying gameplay code.
	// Changes made by listeners will be picked up by the next collision update.
	myPairs = std::move(currentOverlapPairs);

	for (const auto& overlapEvent : pendingOverlapEvents)
	{
		ServiceLocator::GetInstance().GetEventBus().Dispatch(overlapEvent);
	}
}
