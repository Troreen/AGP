#include "GameFramework.pch.h"
#include "GameFramework/Components/ColliderComponent.h"
#include "Maths.hpp"
#include "GraphicsEngine/Drawers/LineDrawer.h"
#include "GameFramework/World/World.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>


void DrawEdge(const CU::Vector3f& startPosition, const CU::Vector3f& endPosition, const CU::Matrix4f& worldMatrix, const CU::Vector4f& color)
{
	const auto start = CU::Vector4f{startPosition.x, startPosition.y, startPosition.z, 1} * worldMatrix;
	const auto end = CU::Vector4f{endPosition.x, endPosition.y, endPosition.z, 1} * worldMatrix;
	LineDrawer::Get().DrawLine(start.XYZ(), end.XYZ(), color);
}

bool ColliderComponent::ourAllDebugDrawEnabled = false;

ColliderComponent* Collision::OtherCollider() const
{
	return SourceWorld->FindCollider(OtherColliderId);
}

Actor* Collision::OtherActor() const
{
	auto* otherCollider = OtherCollider();
	return otherCollider ? otherCollider->GetOwner() : nullptr;
}

void ColliderComponent::SetAllDebugDrawEnabled(bool enabled)
{
	ourAllDebugDrawEnabled = enabled;
}

void ColliderComponent::SetSphereRadius(float radius)
{
	mySphereRadius = radius;
	myShape = ColliderShape::Sphere;
}

void ColliderComponent::SetBoxHalfExtents(const CommonUtilities::Vector3f& halfExtents)
{
	myBoxHalfExtents = halfExtents;
	myShape = ColliderShape::Box;
}

bool ColliderComponent::SetLayer(CollisionLayer layer)
{
	if (static_cast<unsigned int>(layer) > 31)
	{
		return false;
	}
	myLayer = layer;
	return true;
}

bool ColliderComponent::SetOverlapsWith(std::initializer_list<CollisionLayer> layers)
{
	std::uint32_t mask = 0;
	for (CollisionLayer layer : layers)
	{
		const auto layerNumber = static_cast<unsigned int>(layer);
		if (layerNumber > 31)
		{
			return false;
		}
		mask |= std::uint32_t{1} << layerNumber;
	}

	myMask = mask;
	return true;
}

void ColliderComponent::SetOverlapsWithAll()
{
	myMask = UINT32_MAX;
}

void ColliderComponent::SetOverlapsWithNone()
{
	myMask = 0;
}

void ColliderComponent::SetMask(std::uint32_t mask)
{
	myMask = mask;
}

void ColliderComponent::DrawDebug() const
{
	if (!myDebugDrawEnabled && !ourAllDebugDrawEnabled)
	{
		return;
	}
	// Disabled colliders still show their outline so consumed triggers remain visible while debugging.
	const CommonUtilities::Vector4f& color = IsEnabled() ? myDebugColor : myDebugDisabledColor;

	// Transform local outlines with the same composed matrix used for detection.
	const CU::Matrix4f worldMatrix = GetWorldMatrix();

	if (GetShape() == ColliderShape::Sphere)
	{
		constexpr int SegmentCount = 32;
		const float radius = GetSphereRadius();
		for (int segment = 0; segment < SegmentCount; ++segment)
		{
			const float firstAngle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(segment) / SegmentCount;
			const float secondAngle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(segment + 1) / SegmentCount;
			const float x = radius * std::cos(firstAngle);
			const float y = radius * std::sin(firstAngle);
			const float nextX = radius * std::cos(secondAngle);
			const float nextY = radius * std::sin(secondAngle);
			DrawEdge({x, y, 0}, {nextX, nextY, 0}, worldMatrix, color);
			DrawEdge({x, 0, y}, {nextX, 0, nextY}, worldMatrix, color);
			DrawEdge({0, x, y}, {0, nextX, nextY}, worldMatrix, color);
		}
		return;
	}

	// Each bit chooses the positive or negative side of one axis, giving all eight box corners.
	const CU::Vector3f halfExtents = GetBoxHalfExtents();
	std::array<CU::Vector3f, 8> corners;
	for (unsigned corner = 0; corner < corners.size(); ++corner)
	{
		corners[corner] = {(corner & 1) ? halfExtents.x : -halfExtents.x,
			(corner & 2) ? halfExtents.y : -halfExtents.y,
			(corner & 4) ? halfExtents.z : -halfExtents.z};
	}
	// Connect corners that differ on one axis. Starting on the negative side draws each edge once.
	for (unsigned corner = 0; corner < corners.size(); ++corner)
	{
		for (unsigned axisBit : {1u, 2u, 4u})
		{
			if ((corner & axisBit) == 0)
			{
				DrawEdge(corners[corner], corners[corner | axisBit], worldMatrix, color);
			}
		}
	}
}
