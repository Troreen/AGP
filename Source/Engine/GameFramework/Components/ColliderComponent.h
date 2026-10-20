#pragma once
#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/Collision/CollisionSubscription.h"
#include "Vector4.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

enum class ColliderShape
{
	Sphere,
	Box
};

// Detects when shapes overlap without stopping their movement.
class ColliderComponent : public SceneComponent
{
	friend class Actor;
	friend class World;
	friend class CollisionSystem;

public:
	// [[nodiscard]] reminds the caller to store the returned subscription; otherwise listening stops immediately.
	[[nodiscard]] CollisionSubscription OnEnter(std::function<void(const Collision&)> collisionCallback);
	[[nodiscard]] CollisionSubscription OnStay(std::function<void(const Collision&)> collisionCallback);
	[[nodiscard]] CollisionSubscription OnExit(std::function<void(const Collision&)> collisionCallback);

	// Keep the subscription on the receiving Component so its destruction stops listening.
	// Example: mySubscription = hitbox->OnEnter(this, &CombatComponent::OnHit);
	template <class TComponent>
	[[nodiscard]] CollisionSubscription OnEnter(TComponent* listeningComponent, void (TComponent::*receiverMethod)(const Collision&))
	{
		return OnEnter(BindReceiver(listeningComponent, receiverMethod));
	}

	template <class TComponent>
	[[nodiscard]] CollisionSubscription OnStay(TComponent* listeningComponent, void (TComponent::*receiverMethod)(const Collision&))
	{
		return OnStay(BindReceiver(listeningComponent, receiverMethod));
	}

	template <class TComponent>
	[[nodiscard]] CollisionSubscription OnExit(TComponent* listeningComponent, void (TComponent::*receiverMethod)(const Collision&))
	{
		return OnExit(BindReceiver(listeningComponent, receiverMethod));
	}

	// Enables collider outlines for every collider in the process. Intended for debug visualization.
	static void SetAllDebugDrawEnabled(bool enabled);

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
	template <class TComponent>
	static std::function<void(const Collision&)> BindReceiver(TComponent* listeningComponent, void (TComponent::*receiverMethod)(const Collision&))
	{
		return [listeningComponent, receiverMethod](const Collision& collision)
		{
			if (receiverMethod && CanReceiveCollision(listeningComponent))
			{
				(listeningComponent->*receiverMethod)(collision);
			}
		};
	}

	static bool CanReceiveCollision(const Component* listeningComponent);
	CollisionSubscription Subscribe(CollisionOverlapPhase collisionPhase, std::function<void(const Collision&)> collisionCallback);
	void DispatchCollision(const Collision& collision);
	// The returned subscription owns the registration; the collider must not extend its lifetime.
	std::vector<std::weak_ptr<CollisionSubscription::Subscription>> myCollisionSubscriptions;

	void DrawDebug() const;

	static bool ourAllDebugDrawEnabled;
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
