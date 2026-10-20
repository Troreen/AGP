#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

class Actor;
class World;
struct Collision;

// Owned by one Actor. Owner is available after AddComponent returns.
class Component
{
	friend class Actor;
	friend class World;

public:
	virtual ~Component() = default;
	Component() = default;
	Component(const Component&) = delete;
	Component& operator=(const Component&) = delete;

	virtual void BeginPlay()
	{
	}

	virtual void Update(float)
	{
	}

	// Override the phases this Component needs. The World calls them after normal updates
	// when this Actor's collider overlaps another: Enter once, Stay while overlapping, then Exit.
	// Only enabled Components on active Actors receive these calls. No registration is needed.
	virtual void OnCollisionEnter(const Collision&)
	{
	}

	virtual void OnCollisionStay(const Collision&)
	{
	}

	virtual void OnCollisionExit(const Collision&)
	{
	}

	virtual void EndPlay() noexcept
	{
	}

	Actor* GetOwner() const
	{
		return myOwner;
	}

	World& GetWorld() const;

	const std::string& GetName() const
	{
		return myName;
	}
	const std::vector<std::string>& GetTags() const { return myTags; }
	bool HasTag(const std::string& tag) const { return std::find(myTags.begin(), myTags.end(), tag) != myTags.end(); }
	const std::string& GetSourceParent() const { return mySourceParent; }
	void SetSourceMetadata(std::vector<std::string> tags, std::string sourceParent)
	{
		myTags = std::move(tags);
		mySourceParent = std::move(sourceParent);
	}

	bool HasBegunPlay() const
	{
		return myBegun;
	}

	bool IsPendingDestroy() const
	{
		return myDestroyed;
	}

	bool IsEnabled() const
	{
		return myEnabled && !myDestroyed;
	}

	void SetEnabled(bool enabled)
	{
		myEnabled = enabled;
	}

	// Stops updates immediately; memory is released at the next World update.
	void Destroy()
	{
		myDestroyed = true;
	}

private:
	Actor* myOwner = nullptr;
	std::string myName;
	std::vector<std::string> myTags;
	std::string mySourceParent;
	bool myEnabled = true;
	bool myBegun = false;
	bool myDestroyed = false;
};
