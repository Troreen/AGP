#pragma once
#include "GameFramework/Collision/CollisionOverlapEvent.h"
#include <functional>
#include <memory>

// Keeps one collision callback registered until Reset or destruction.
// Store the subscription on the listening Component. When the subscription goes away, listening stops.
class CollisionSubscription
{
	friend class ColliderComponent;

public:
	CollisionSubscription() = default;
	~CollisionSubscription();
	CollisionSubscription(CollisionSubscription&& otherSubscription) noexcept;
	CollisionSubscription& operator=(CollisionSubscription&& otherSubscription) noexcept;
	CollisionSubscription(const CollisionSubscription&) = delete;
	CollisionSubscription& operator=(const CollisionSubscription&) = delete;
	// Stop now, usually in EndPlay. Calling Reset more than once is safe.
	void Reset() noexcept;

private:
	struct Subscription
	{
		std::function<void(const Collision&)> Callback;
		CollisionOverlapPhase Phase;
		bool Connected = true;
	};

	explicit CollisionSubscription(std::shared_ptr<Subscription> collisionSubscription);
	std::shared_ptr<Subscription> mySubscription;
};
