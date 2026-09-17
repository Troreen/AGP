#pragma once
#include "GameFramework/Runtime/Internal/GameLoop.h"
#include "GameFramework/Runtime/Internal/WorldAccess.h"
#include <stdexcept>

// CPU fixture uses the same frame boundary and phase driver as GameApplication.
// Public gameplay examples receive only World; only this fixture drives its lifetime.
class TestSession
{
public:
    TestSession() : myWorld(&myInput), myLoop(.01f) {}
    World& GetWorld() { return myWorld; }
    template<class Compose> void Initialize(Compose compose)
    {
        compose(myWorld);
        SceneDiagnostics diagnostics;
        if (!GameFrameworkInternal::WorldAccess::Prepare(myWorld,diagnostics))
            throw std::runtime_error("Test session startup validation failed");
        GameFrameworkInternal::WorldAccess::Activate(myWorld);
    }
    bool Step(float delta,const GameInput& input={})
    {
        myInput=input;
        SceneDiagnostics diagnostics;
        const bool accepted=GameFrameworkInternal::WorldAccess::Flush(myWorld,diagnostics);
        myLoop.Advance(delta,input,
            [&](float dt,const GameInput& sample) { myInput=sample;GameFrameworkInternal::WorldAccess::FixedUpdate(myWorld,dt); },
            [&](float dt,const GameInput& sample) { myInput=sample;GameFrameworkInternal::WorldAccess::Update(myWorld,dt); },
            [](float,const GameInput&) {});
        return accepted;
    }
private:
    GameInput myInput;
    World myWorld;
    GameFrameworkInternal::GameLoop myLoop;
};
