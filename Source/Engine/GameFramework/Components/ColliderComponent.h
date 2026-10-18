#pragma once
#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/Collision/CollisionOverlapEvent.h"
#include "Vector4.hpp"

enum class ColliderShape
{
	Sphere,
	Box
};

// Nonblocking geometry. Gameplay supplies meaning through overlap subscriptions.
class ColliderComponent : public SceneComponent
{
	friend class Actor;
	friend class World;

public:
	bool SetSphereRadius(float radius);
	bool SetBoxHalfExtents(const CommonUtilities::Vector3f& halfExtents);
	bool SetLayer(unsigned int layer);
	bool SetMask(std::uint32_t mask);

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

	unsigned int GetLayer() const
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

	bool myDebugDrawEnabled = false;
	CommonUtilities::Vector4f myDebugColor{0, 1, 0, 1};
	CommonUtilities::Vector4f myDebugDisabledColor{0.4f, 0.4f, 0.4f, 1};
	ColliderId myColliderId = 0;
	float mySphereRadius = 1.0f;
	CommonUtilities::Vector3f myBoxHalfExtents{1, 1, 1};
	ColliderShape myShape = ColliderShape::Sphere;
	unsigned int myLayer = 0;
	std::uint32_t myMask = UINT32_MAX;
};
