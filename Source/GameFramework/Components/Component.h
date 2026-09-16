#pragma once

#include <string>

class Actor;

// Base for engine features and game-authored behavior. Override only the phases
// you need; the defaults do nothing. World/Actor call these hooks automatically
// when the actor is active and the component enabled. Keep game rules in the game
// project even though they derive from this reusable framework type.
class Component
{
public:
	virtual ~Component() = default;

	// Constant-step simulation; zero to five calls per gameplay frame with current policy.
	virtual void FixedUpdate(float aDeltaTime);
	// Frame-time behavior. All components finish this phase before any LateUpdate.
	virtual void Update(float aDeltaTime);
	// Post-update adjustments, such as cameras that depend on the completed pose.
	virtual void LateUpdate(float aDeltaTime);
	// Called by Actor before removal/destruction. This is not scene EndPlay. Avoid
	// assuming sibling components still exist; teardown invalidates borrowed references.
	virtual void OnDestroy();
	virtual void OnActiveChanged(bool anIsActive);
	virtual void OnEnabledChanged(bool anIsEnabled);

	const std::string& GetName() const;
	// Assigned after construction by AddComponent; null inside the component constructor.
	Actor* GetOwner() const;

	bool IsEnabled() const;
	void SetEnabled(bool anIsEnabled);

private:
	void SetOwner(Actor* anOwner);
	void SetName(std::string aName);

	std::string myName;
	Actor* myOwner = nullptr;
	bool myIsEnabled = true;

	friend class Actor;
};
