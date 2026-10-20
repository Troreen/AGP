#include "GameFramework/Collision/CollisionSystem.h"
#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"
#include "GameFramework/EventBus.hpp"
#include "GameFramework/ServiceLocator.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
	void DispatchCollision(World& world, ColliderId receivingColliderId, const Collision& collision, const std::vector<Component*>& frameComponents)
	{
		auto* collider = world.FindCollider(receivingColliderId);
		if (!collider)
		{
			return;
		}
		Actor* owner = collider->GetOwner();

		for (Component* component : frameComponents)
		{
			if (collider->IsPendingDestroy() || !owner->IsActive())
			{
				break;
			}

			if (component->GetOwner() != owner || !component->IsEnabled())
			{
				continue;
			}

			switch (collision.Phase)
			{
				case CollisionOverlapPhase::Enter:
				{
					component->OnCollisionEnter(collision);
					break;
				}
				case CollisionOverlapPhase::Stay:
				{
					component->OnCollisionStay(collision);
					break;
				}
				case CollisionOverlapPhase::Exit:
				{
					component->OnCollisionExit(collision);
					break;
				}
			}
		}
	}

	using CollisionVector = CommonUtilities::Vector3<double>;

	// Allow a little rounding error so shapes that just touch still count as overlapping.
	constexpr double TouchingTolerance = 1.000001;

	// A copy of the collider's size, position and directions for this frame's overlap tests.
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

	struct TransformAxesAndScale
	{
		CollisionVector NormalizedAxes[3];
		CollisionVector AxisScales;
	};

	TransformAxesAndScale ReadTransformAxesAndScale(const CommonUtilities::Matrix4f& transformMatrix)
	{
		// Each row gives a direction and its scale. Remove the scale to get unit directions.
		CollisionVector normalizedAxes[3];
		normalizedAxes[0] = {transformMatrix(1, 1), transformMatrix(1, 2), transformMatrix(1, 3)};
		normalizedAxes[1] = {transformMatrix(2, 1), transformMatrix(2, 2), transformMatrix(2, 3)};
		normalizedAxes[2] = {transformMatrix(3, 1), transformMatrix(3, 2), transformMatrix(3, 3)};
		const CollisionVector axisScales = {normalizedAxes[0].Length(), normalizedAxes[1].Length(), normalizedAxes[2].Length()};

		normalizedAxes[0] /= axisScales.x;
		normalizedAxes[1] /= axisScales.y;
		normalizedAxes[2] /= axisScales.z;

		return TransformAxesAndScale{{normalizedAxes[0], normalizedAxes[1], normalizedAxes[2]}, axisScales};
	}

	WorldColliderShape BuildWorldColliderShape(const ColliderComponent& collider)
	{
		const auto transformMatrix = collider.GetWorldMatrix();
		const auto worldAxesAndScale = ReadTransformAxesAndScale(transformMatrix);
		const auto& axisScales = worldAxesAndScale.AxisScales;
		const auto& normalizedAxes = worldAxesAndScale.NormalizedAxes;

		// Put the dimensions into world units once. The overlap tests then use these values directly.
		const auto& localBoxHalfExtents = collider.GetBoxHalfExtents();
		return WorldColliderShape
		{
			collider.GetColliderId(),
			static_cast<unsigned int>(collider.GetLayer()),
			collider.GetMask(),
			{transformMatrix(4, 1), transformMatrix(4, 2), transformMatrix(4, 3)},
			collider.GetSphereRadius() * axisScales.x,
			collider.GetShape(),
			{localBoxHalfExtents.x * axisScales.x, localBoxHalfExtents.y * axisScales.y, localBoxHalfExtents.z * axisScales.z},
			normalizedAxes[0],
			normalizedAxes[2]
		};
	}

	bool SphereOverlapsBox(const WorldColliderShape& sphereShape, const WorldColliderShape& boxShape)
	{
		// Closest-point test: https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#sphere_vs._aabb
		// Here we measure along the box's own directions so the same idea works for a yaw-rotated box.
		// Find how far the sphere center sits outside the box in each direction.
		// Inside the box, that distance is zero. The remaining gap must fit within the sphere radius.

		const CollisionVector centerOffset = sphereShape.Center - boxShape.Center;
		const double distanceOutsideRightFace = (std::max)(0.0, std::abs(centerOffset.Dot(boxShape.Right)) - boxShape.HalfExtents.x);
		const double distanceOutsideTopFace = (std::max)(0.0, std::abs(centerOffset.y) - boxShape.HalfExtents.y);
		const double distanceOutsideForwardFace = (std::max)(0.0, std::abs(centerOffset.Dot(boxShape.Forward)) - boxShape.HalfExtents.z);

		// Compare squared distances to avoid a square root. A sphere reaches the box if its radius covers the gap.
		const double allowedSphereRadius = sphereShape.Radius * TouchingTolerance;

		return distanceOutsideRightFace * distanceOutsideRightFace + distanceOutsideTopFace * distanceOutsideTopFace +
		           distanceOutsideForwardFace * distanceOutsideForwardFace <=
		       allowedSphereRadius * allowedSphereRadius;
	}

	double GetBoxRadiusAlongAxis(const WorldColliderShape& worldShape, const CollisionVector& projectionAxis)
	{
		// Measure how far the box reaches from its center when viewed along this direction.
		// Both its width and depth contribute when the box is turned.
		return worldShape.HalfExtents.x * std::abs(worldShape.Right.Dot(projectionAxis)) +
		       worldShape.HalfExtents.z * std::abs(worldShape.Forward.Dot(projectionAxis));
	}

	bool BoxesOverlap(const WorldColliderShape& firstShape, const WorldColliderShape& secondShape)
	{
		// Separating Axis Theorem (SAT): https://dyn4j.org/2010/01/sat/
		// For upright boxes, test their height and the four rectangle directions seen from above.
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
		// Pick the test for these shapes. The sphere/box test always takes the sphere first.
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

		// Sphere distance test: https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#sphere_vs._sphere
		// Two spheres touch when the distance between their centers reaches the sum of their radii.
		const double combinedSphereRadius = (firstShape.Radius + secondShape.Radius) * TouchingTolerance;
		return (firstShape.Center - secondShape.Center).LengthSqr() <= combinedSphereRadius * combinedSphereRadius;
	}
}

void CollisionSystem::RegisterCollider(ColliderComponent& collider)
{
	if (collider.GetOwner()->GetComponent<ColliderComponent>())
	{
		throw std::invalid_argument("An Actor can own only one collider");
	}
	
	collider.myColliderId = myNextColliderId++;
}

void CollisionSystem::Update(World& world, const std::vector<Component*>& frameComponents)
{
	// Gather the colliders that can take part this frame, using their positions after gameplay updates.
	// Keep copies of their shapes so callbacks cannot change the overlaps we are about to collect.
	std::vector<WorldColliderShape> worldColliderShapes;
	for (Component* component : frameComponents)
	{
		if (!component->IsEnabled() || !component->GetOwner()->IsActive())
		{
			continue;
		}

		if (auto* collider = dynamic_cast<ColliderComponent*>(component))
		{
			worldColliderShapes.push_back(BuildWorldColliderShape(*collider));
		}
	}

	// Check each pair once. Both colliders must allow the other's layer.
	std::set<std::pair<ColliderId, ColliderId>> currentOverlapPairs;
	std::vector<CollisionOverlapEvent> pendingOverlapEvents;
	// TODO: Consider using a spatial partitioning structure to avoid O(n^2) checks for large numbers of colliders.
	for (size_t firstShapeIndex = 0; firstShapeIndex < worldColliderShapes.size(); ++firstShapeIndex)
	{
		for (size_t secondShapeIndex = firstShapeIndex + 1; secondShapeIndex < worldColliderShapes.size(); ++secondShapeIndex)
		{
			const WorldColliderShape& firstShape = worldColliderShapes[firstShapeIndex];
			const WorldColliderShape& secondShape = worldColliderShapes[secondShapeIndex];
			// Each layer has one bit in the mask. Both sides must allow the other before testing their shapes.
			if (!(firstShape.Mask & (std::uint32_t{1} << secondShape.Layer)) ||
			    !(secondShape.Mask & (std::uint32_t{1} << firstShape.Layer)) || !Overlaps(firstShape, secondShape))
			{
				continue;
			}

			// Keep the smaller ID first so the same pair is recognized regardless of which shape came first.
			const auto overlapPair = std::minmax(firstShape.Id, secondShape.Id);
			currentOverlapPairs.emplace(overlapPair.first, overlapPair.second);

			// A pair from last frame is still inside; a pair we have not seen before is entering.
			pendingOverlapEvents.push_back({&world, overlapPair.first, overlapPair.second,
			                                myPairs.contains(overlapPair) ? CollisionOverlapPhase::Stay : CollisionOverlapPhase::Enter});
		}
	}

	// Any old pair missing from this frame has separated or lost a collider that could take part.
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

	// Tell both Actors about each overlap. Each side sees the other collider in its Collision details.
	// Then send the same pair to EventBus listeners that watch collisions across the World.
	for (const auto& overlapEvent : pendingOverlapEvents)
	{
		DispatchCollision(world, overlapEvent.FirstColliderId, {&world, overlapEvent.SecondColliderId, overlapEvent.Phase}, frameComponents);
		DispatchCollision(world, overlapEvent.SecondColliderId, {&world, overlapEvent.FirstColliderId, overlapEvent.Phase}, frameComponents);
		ServiceLocator::GetInstance().GetEventBus().Dispatch(overlapEvent);
	}
}
