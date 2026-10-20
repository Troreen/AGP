#pragma once
#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/Collision/CollisionOverlapEvent.h"
#include "GameFramework/Collision/CollisionLayers.h"
#include "Vector4.hpp"
#include <cstdint>
#include <initializer_list>

enum class ColliderShape
{
	Sphere,
	Box
};

// Detects when shapes overlap without stopping their movement.
class ColliderComponent : public SceneComponent
{
	friend class World;
	friend class CollisionSystem;

public:
	// Enables collider outlines for every collider in the process. Intended for debug visualization.
	static void SetAllDebugDrawEnabled(bool enabled);

	// Setting dimensions also selects the shape. Supply a positive radius or positive box half-extents.
	void SetSphereRadius(float radius);
	void SetBoxHalfExtents(const CommonUtilities::Vector3f& halfExtents);
	// The layer says what this collider is; the accepted layers say what it can overlap.
	bool SetLayer(CollisionLayer layer);

	// Replaces the accepted layers. Both colliders must accept each other's layer.
	bool SetOverlapsWith(std::initializer_list<CollisionLayer> layers);

	void SetOverlapsWithAll();
	void SetOverlapsWithNone();

	// Raw mask access. Gameplay can use SetOverlapsWith.
	void SetMask(std::uint32_t mask);

	// Drawing is independent of collision participation, so disabled volumes remain visible.
	void SetDebugDrawEnabled(bool enabled)
	{
		myDebugDrawEnabled = enabled;
	}

	bool IsDebugDrawEnabled() const
	{
		return myDebugDrawEnabled;
	}

	void SetDebugColor(const CommonUtilities::Vector4f& color)
	{
		myDebugColor = color;
	}

	const CommonUtilities::Vector4f& GetDebugColor() const
	{
		return myDebugColor;
	}

	void SetDebugDisabledColor(const CommonUtilities::Vector4f& color)
	{
		myDebugDisabledColor = color;
	}

	const CommonUtilities::Vector4f& GetDebugDisabledColor() const
	{
		return myDebugDisabledColor;
	}

	float GetSphereRadius() const
	{
		return mySphereRadius;
	}

	const CommonUtilities::Vector3f& GetBoxHalfExtents() const
	{
		return myBoxHalfExtents;
	}

	CollisionLayer GetLayer() const
	{
		return myLayer;
	}

	std::uint32_t GetMask() const
	{
		return myMask;
	}

	ColliderId GetColliderId() const
	{
		return myColliderId;
	}

	ColliderShape GetShape() const
	{
		return myShape;
	}

private:
	void DrawDebug() const;

	static bool ourAllDebugDrawEnabled;
	bool myDebugDrawEnabled = false;
	CommonUtilities::Vector4f myDebugColor{0, 1, 0, 1};
	CommonUtilities::Vector4f myDebugDisabledColor{0.4f, 0.4f, 0.4f, 1};
	ColliderId myColliderId = 0;
	float mySphereRadius = 1.0f;
	CommonUtilities::Vector3f myBoxHalfExtents{1, 1, 1};
	ColliderShape myShape = ColliderShape::Sphere;
	CollisionLayer myLayer = CollisionLayer::Default;
	std::uint32_t myMask = UINT32_MAX;
};
