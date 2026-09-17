#pragma once

#include <string>
#include "GameFramework/ObjectRef.h"
#include "GameFramework/SceneDiagnostic.h"

class Actor;
class World;
class References;
struct GameInput;
class SceneService;
class AssetLookup;
class GameTime;
namespace GameFrameworkInternal { class WorldAccess; }

// Base for engine features and game-authored behavior. Override only the phases
// you need; the defaults do nothing. World/Actor call these hooks automatically
// when the actor is active and the component enabled. Keep game rules in the game
// project even though they derive from this reusable framework type.
class Component
{
public:
	Component() = default;
    virtual ~Component() = default;
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    // ResolveReferences validates dependencies after all objects have been configured. BeginPlay
    // starts behavior only after the entire batch passes validation, even if disabled.
    virtual void ResolveReferences(References&) {}
    virtual void BeginPlay() {}
    virtual void EndPlay() {}
    World& GetWorld() const;
    const GameInput& GetInput() const;
    SceneService& GetScenes() const;
    const AssetLookup& GetAssets() const;
    const GameTime& GetTime() const;
    void Destroy();
    bool HasBegunPlay() const { return myBegun; }
    bool IsPendingDestroy() const { return myPendingDestroy; }
    template<class T = Component> ComponentRef<T> GetRef() const { return ComponentRef<T>(myHandle); }

	// Constant-step simulation; zero to five calls per gameplay frame with current policy.
	virtual void FixedUpdate(float aDeltaTime);
	// Frame-time behavior. All components finish this phase before any LateUpdate.
	virtual void Update(float aDeltaTime);
	// Post-update adjustments, such as cameras that depend on the completed pose.
	virtual void LateUpdate(float aDeltaTime);
    const std::string& GetName() const;
	// Assigned after construction by AddComponent; null inside the component constructor.
	Actor* GetOwner() const;

	bool IsEnabled() const;
	void SetEnabled(bool anIsEnabled);

protected:
    void EnsureCanMutate() const;
private:
	void SetOwner(Actor* anOwner);
	void SetName(std::string aName);

	std::string myName;
	Actor* myOwner = nullptr;
	bool myIsEnabled = true;
    bool myPendingDestroy = false;
    bool myConnected = false;
    bool myAdmitted = false;
    bool myBegun = false;
    GameFrameworkInternal::ObjectIdentity myHandle;
    SceneDiagnostic mySourceDiagnostic;
    friend class References;
    friend class GameFrameworkInternal::WorldAccess;
    friend class World;
    friend class SceneComponent;

	friend class Actor;
};
