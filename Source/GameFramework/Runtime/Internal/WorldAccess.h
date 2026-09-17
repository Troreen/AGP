#pragma once
#include "../../World/World.h"

namespace GameFrameworkInternal
{
    // Host, construction and CPU-test access. Never included by gameplay headers.
    class WorldAccess
    {
    public:
        using State = World::State;
        static bool Prepare(World& world, SceneDiagnostics& diagnostics) { return world.Prepare(diagnostics); }
        static void Activate(World& world) { world.Activate(); }
        static bool Flush(World& world, SceneDiagnostics& diagnostics) { return world.Flush(diagnostics); }
        static void Close(World& world) noexcept { world.myClosing = true; }
        static void Shutdown(World& world) noexcept { world.Shutdown(); }
        static void FixedUpdate(World& world, float delta) { world.FixedUpdate(delta); }
        static void Update(World& world, float delta) { world.Update(delta); }
        static State GetState(const World& world) { return world.GetState(); }
        static bool AcceptsChanges(const World& world) { return world.AcceptsChanges(); }
        static const std::vector<std::unique_ptr<Actor>>& GetActors(const World& world) { return world.GetActors(); }
        static const std::vector<std::unique_ptr<Component>>& GetComponents(const Actor& actor) { return actor.GetComponents(); }
    };
}
