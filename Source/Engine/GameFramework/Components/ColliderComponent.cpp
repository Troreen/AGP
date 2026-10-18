#include "GameFramework/Components/ColliderComponent.h"
#include "Maths.hpp"
#include "GraphicsEngine/Drawers/LineDrawer.h"
#include <array>
#include <cmath>
#include <numbers>

bool ColliderComponent::SetSphereRadius(float radius)
{
	if (!std::isfinite(radius) || radius <= 0)
	{
		return false;
	}
	mySphereRadius = radius;
	myShape = ColliderShape::Sphere;
	return true;
}

bool ColliderComponent::SetBoxHalfExtents(const CommonUtilities::Vector3f& halfExtents)
{
	if (!std::isfinite(halfExtents.x) || !std::isfinite(halfExtents.y) || !std::isfinite(halfExtents.z) || halfExtents.x <= 0 ||
	    halfExtents.y <= 0 || halfExtents.z <= 0)
	{
		return false;
	}
	myBoxHalfExtents = halfExtents;
	myShape = ColliderShape::Box;
	return true;
}

bool ColliderComponent::SetLayer(unsigned int layer)
{
	if (layer > 31)
	{
		return false;
	}
	myLayer = layer;
	return true;
}

bool ColliderComponent::SetMask(std::uint32_t mask)
{
	myMask = mask;
	return true;
}

void ColliderComponent::DrawDebug() const
{
	if (!myDebugDrawEnabled)
	{
		return;
	}
	const CommonUtilities::Vector4f& color = IsEnabled() ? myDebugColor : myDebugDisabledColor;

	// Transform local outlines with the same composed matrix used for detection.
	const CU::Matrix4f worldMatrix = GetWorldMatrix();
	auto drawEdge = [&](const CU::Vector3f& aStart, const CU::Vector3f& anEnd)
	{
		const auto start = CU::Vector4f{aStart.x, aStart.y, aStart.z, 1} * worldMatrix;
		const auto end = CU::Vector4f{anEnd.x, anEnd.y, anEnd.z, 1} * worldMatrix;
		LineDrawer::Get().DrawLine(start.XYZ(), end.XYZ(), color);
	};

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
			drawEdge({x, y, 0}, {nextX, nextY, 0});
			drawEdge({x, 0, y}, {nextX, 0, nextY});
			drawEdge({0, x, y}, {0, nextX, nextY});
		}
		return;
	}

	const CU::Vector3f halfExtents = GetBoxHalfExtents();
	std::array<CU::Vector3f, 8> corners;
	for (unsigned corner = 0; corner < corners.size(); ++corner)
	{
		corners[corner] = {(corner & 1) ? halfExtents.x : -halfExtents.x,
			(corner & 2) ? halfExtents.y : -halfExtents.y,
			(corner & 4) ? halfExtents.z : -halfExtents.z};
	}
	for (unsigned corner = 0; corner < corners.size(); ++corner)
	{
		for (unsigned axisBit : {1u, 2u, 4u})
		{
			if ((corner & axisBit) == 0)
			{
				drawEdge(corners[corner], corners[corner | axisBit]);
			}
		}
	}
}
