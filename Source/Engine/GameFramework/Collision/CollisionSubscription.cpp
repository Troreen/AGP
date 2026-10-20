#include "GameFramework/Collision/CollisionSubscription.h"
#include <utility>

CollisionSubscription::CollisionSubscription(std::shared_ptr<Subscription> collisionSubscription)
    : mySubscription(std::move(collisionSubscription))
{
}

CollisionSubscription::~CollisionSubscription()
{
	Reset();
}

CollisionSubscription::CollisionSubscription(CollisionSubscription&& otherSubscription) noexcept
    : mySubscription(std::move(otherSubscription.mySubscription))
{
}

CollisionSubscription& CollisionSubscription::operator=(CollisionSubscription&& otherSubscription) noexcept
{
	if (this != &otherSubscription)
	{
		Reset();
		mySubscription = std::move(otherSubscription.mySubscription);
	}
	return *this;
}

void CollisionSubscription::Reset() noexcept
{
	if (mySubscription)
	{
		// Stop delivery even if an in-progress dispatch still holds this registration.
		mySubscription->Connected = false;
		mySubscription.reset();
	}
}
