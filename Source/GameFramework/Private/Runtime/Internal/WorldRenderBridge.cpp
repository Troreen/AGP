#include "Runtime/Internal/WorldRenderBridge.h"
#include "Runtime/Internal/WorldAccess.h"
#include "Runtime/Internal/RenderAccess.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/MeshComponentBase.h"
#include <chrono>

namespace
{
    RenderLightType RenderType(LightType type)
    {
        switch (type)
        {
        case LightType::Directional: return RenderLightType::Directional;
        case LightType::Point: return RenderLightType::Point;
        case LightType::Spot: return RenderLightType::Spot;
        }
        return static_cast<RenderLightType>(static_cast<uint32_t>(type));
    }
    GraphicsEngine::LightSnapshot CopyLight(const LightComponent& component)
    {
        GraphicsEngine::LightSnapshot result;
        result.Type = RenderType(component.GetLightType());
        result.Color = component.GetColor();
        result.Intensity = component.GetIntensity();
        result.Position = component.GetWorldPosition();
        result.Direction = component.GetWorldDirection();
        result.InnerCone = component.GetInnerCone();
        result.OuterCone = component.GetOuterCone();
        result.Radius = component.GetRadius();
        return result;
    }
}

bool GameFrameworkInternal::WorldRenderBridge::Build(const World& world, GraphicsEngine& graphics,
    GraphicsEngine::RenderSceneSnapshot& snapshot)
{
    const auto start = std::chrono::steady_clock::now();
    snapshot.Clear();
    auto* camera = world.GetActiveCamera();
    if (!camera || !camera->HasBegunPlay() || !camera->IsEnabled() || !camera->GetOwner()->IsActiveInHierarchy()) return false;
    snapshot.Camera = RenderAccess::Camera(*camera);
    snapshot.HasCamera = true;
    const auto& actors = WorldAccess::GetActors(world);
    snapshot.ShadowCasters.reserve(actors.size());
    snapshot.RelevantLights.reserve(actors.size());
    std::vector<LightComponent*> lights;
    std::vector<MeshComponentBase*> meshes;
    for (const auto& actor : actors)
    {
        if (!actor || !actor->IsActiveInHierarchy()) continue;
        lights.clear(); actor->GetComponentsOfType(lights);
        for (const auto* light : lights)
            if (light && light->HasBegunPlay() && light->IsEnabled()) snapshot.RelevantLights.push_back(CopyLight(*light));
        meshes.clear(); actor->GetComponentsOfType(meshes);
        for (const auto* component : meshes)
        {
            if (!component || !component->HasBegunPlay() || !component->IsEnabled() || !component->IsVisible()) continue;
            auto mesh = RenderAccess::Mesh(*component);
            if (!mesh) continue;
            GraphicsEngine::RenderItemSnapshot item;
            item.Mesh = std::move(mesh);
            item.Materials = RenderAccess::Materials(*component);
            item.World = component->GetWorldMatrix();
            item.HasSkinning = RenderAccess::HasSkinning(*component);
            if (const auto* joints = RenderAccess::JointTransforms(*component)) item.JointTransforms = *joints;
            snapshot.ShadowCasters.push_back(std::move(item));
        }
    }
    graphics.FinalizeRenderSnapshot(snapshot);
    snapshot.Stats.SnapshotMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return true;
}
