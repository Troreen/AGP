#pragma once
#include <string>

class Actor;
class World;
class GameInput;

// Owned by one Actor. Owner and input are available after AddComponent returns.
class Component
{
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

	virtual void EndPlay() noexcept
	{
	}

	Actor* GetOwner() const
	{
		return myOwner;
	}

	World& GetWorld() const;
	const GameInput& GetInput() const;

	const std::string& GetName() const
	{
		return myName;
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
	bool myEnabled = true;
	bool myBegun = false;
	bool myDestroyed = false;
	friend class Actor;
	friend class World;
};
