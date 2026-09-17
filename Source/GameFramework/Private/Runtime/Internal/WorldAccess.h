#pragma once
#include "GameFramework/World.h"
#include <stdexcept>

namespace GameFrameworkInternal
{
    // Host, construction and CPU-test access. Never included by gameplay headers.
    class WorldAccess
    {
    public:
        using State = World::State;
        static std::unique_ptr<World> Create(const GameInput* input = nullptr) { return std::unique_ptr<World>(new World(input)); }
        static bool Prepare(World& world, SceneDiagnostics& diagnostics) { return world.Prepare(diagnostics); }
        static void Activate(World& world) { world.Activate(); }
        static bool Flush(World& world, SceneDiagnostics& diagnostics) { return world.Flush(diagnostics); }
        static void EnsureMutationAllowed(const World& world) { world.EnsureMutationAllowed(); }
        static void Configure(Component& component, const std::function<void()>& callback);
        static void Construct(const std::function<void()>& callback, const char* phase = "Component factory");
        static Component* AttachComponent(Actor& actor, std::string name, std::unique_ptr<Component> component)
        {
            if (!component) throw std::invalid_argument("Factory returned no component");
            if (!actor.CanAttach()) throw std::logic_error("Actor cannot accept components");
            if (!actor.CanAddComponentName(name)) actor.ReportDuplicateComponentName(name);
            auto* result = component.get();
            result->SetOwner(&actor); result->SetName(std::move(name));
            actor.AttachComponent(std::move(component));
            return result;
        }
        static void SetSource(Component& component, SceneDiagnostic source) { component.mySourceDiagnostic = std::move(source); }
        static void BindServices(World& world, SceneService* scenes, const AssetLookup* assets, const GameTime* time)
        { world.myScenes = scenes; world.myAssets = assets; world.myTime = time; }
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
