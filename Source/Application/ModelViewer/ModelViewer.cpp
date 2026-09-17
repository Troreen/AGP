#include "ModelViewer.h"
#include "GameFramework/Scenes/SceneService.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/World/World.h"
#include "Application.h"
#include "ModelViewerComponents.h"
#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Scenes/SceneReader.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"

ModelViewer::ModelViewer() = default;
ModelViewer::~ModelViewer() = default;

// Request the authored sample. The installed source and engine handle construction.
void ModelViewer::Initialize(GameContext& context)
{
    context.GetScenes().Load(SceneId{"ModelViewer"});
	MVLOG(Log, "Game ready: RMB + WASD/Space/Ctrl camera, R pause chest spin, numpad 0-3 animation, 7-9 lights, Shift+7-9 place lights, F5 reload, F7 spawn/destroy hierarchy, Esc quit");
}

// Global session behavior belongs here. Per-actor behavior is attached as components
// and will run automatically after this hook; do not call component Update yourself.
void ModelViewer::Update(GameContext& context, float)
{
	// Session-level rules live here. Actor behavior is ticked by the world.
	if (context.GetInput().IsKeyPressed(Keys::ESCAPE)) context.RequestQuit();
    if (context.GetInput().IsKeyPressed(Keys::F5)) context.GetScenes().Reload();
    if (context.GetInput().IsKeyPressed(Keys::F7))
    {
        auto& world = context.GetWorld();
        if (auto* demo = world.FindActor("Hierarchy Demo")) demo->Destroy();
        else if (auto* source = world.FindActor("SM_Chest Actor"))
        {
            auto* original = source->GetComponent<StaticMeshComponent>();
            auto* demo = world.CreateActor("Hierarchy Demo");
            demo->SetPosition({350,0,285}); demo->AddComponent<SpinComponent>("Spin");
            auto* pivot = demo->AddComponent<SceneComponent>("Pivot");
            pivot->GetLocalTransform().SetPosition({0,50,0});
            for (int i = 0; original && i < 2; ++i)
            {
                auto* mesh = demo->AddComponent<StaticMeshComponent>("Chest " + std::to_string(i),original->GetMesh());
                mesh->GetLocalTransform().SetPosition({i == 0 ? -60.f : 60.f,0,0});
                mesh->GetLocalTransform().SetScale({.5f,.5f,.5f});
                mesh->SetParent(pivot,ReparentMode::KeepLocal);
                const auto& materials = original->GetMaterialList();
                for (unsigned slot = 0; slot < materials.size(); ++slot) mesh->SetMaterial(slot,materials[slot]);
            }
        }
    }
}

// The host has joined gameplay; the world remains borrowable during Shutdown.
void ModelViewer::Shutdown(GameContext& context)
{
	context.SetActiveCamera(nullptr);
}

void ModelViewer::RegisterComponents(ComponentRegistry& registry)
{
    registry.Register<CameraControlsComponent>("CameraControls");
    registry.Register<AnimationControlsComponent>("AnimationControls");
    registry.Register<SpinComponent>("Spin");
    registry.Register<LightControlsComponent>("LightControls", [](LightControlsComponent& c, SceneReader& fields)
    {
        fields.BindActor("camera", c.Camera, ReferenceRequirement::Required);
        fields.BindComponent("directional", c.Directional, ReferenceRequirement::Required);
        fields.BindComponent("point", c.Point, ReferenceRequirement::Required);
        fields.BindComponent("spot", c.Spot, ReferenceRequirement::Required);
    });
}
