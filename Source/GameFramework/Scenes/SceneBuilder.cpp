#include "SceneBuilder.h"
#include <unordered_set>
#include <cmath>

SceneBuildResult SceneBuilder::Build(const SceneDescription& scene, const ComponentRegistry& registry, const GameInput* input)
{
    SceneBuildResult result;
    auto error = [&](std::string actor, std::string component, std::string property, std::string message)
    { result.Diagnostics.push_back({std::move(actor),std::move(component),std::move(property),std::move(message)}); };
    if (!registry.IsFrozen()) error({},{},"registry","Registry must be frozen");
    auto finiteTransform = [](const CommonUtilities::Transform& transform)
    {
        const auto& matrix = transform.GetLocalMatrix();
        for (int r=1;r<=4;++r) for (int c=1;c<=4;++c) if (!std::isfinite(matrix(r,c))) return false;
        return true;
    };
    std::unordered_set<std::string> identities;
    for (const auto& a : scene.Actors)
    {
        if (!finiteTransform(a.LocalTransform)) error(a.Id,{},"transform","Actor transform contains nonfinite values");
        if (a.Id.empty() || !identities.insert(a.Id).second) error(a.Id,{},"id","Empty or duplicate actor ID");
        std::unordered_set<std::string> names;
        for (const auto& c : a.Components)
        {
            if (!finiteTransform(c.LocalTransform)) error(a.Id,c.Name,"transform","Component transform contains nonfinite values");
            if (c.Name.empty() || !names.insert(c.Name).second) error(a.Id,c.Name,"name","Empty or duplicate component name");
            if (!registry.Contains(c.Type)) error(a.Id,c.Name,"type","Unknown registered type: " + c.Type);
        }
    }
    if (!result.Diagnostics.empty()) return result;
    auto candidate = std::make_unique<World>(input);
    try
    {
        // First attach every object so configuration and Connect can use forward references.
        for (const auto& a : scene.Actors)
        {
            auto* actor = candidate->CreateActor(a.Id);
            actor->SetLocalPose({a.LocalTransform.GetPosition(), a.LocalTransform.GetRotation(), a.LocalTransform.GetScale()});
            actor->SetActive(a.Active);
            for (const auto& c : a.Components) registry.Create(c.Type,*actor,c.Name);
        }
        for (const auto& a : scene.Actors)
        {
            auto* actor = candidate->FindActor(a.Id);
            if (!a.Parent.empty())
            {
                auto* parent = candidate->FindActor(a.Parent);
                if (!parent || !actor->SetParent(parent,ReparentMode::KeepLocal)) error(a.Id,{},"parent","Missing or cyclic actor parent");
            }
            for (const auto& c : a.Components)
            {
                auto* component = actor->FindComponent(c.Name);
                component->SetEnabled(c.Enabled);
                if (auto* spatial = dynamic_cast<SceneComponent*>(component))
                {
                    // Copy values, never a source Transform's raw parent pointer.
                    spatial->GetLocalTransform().SetPosition(c.LocalTransform.GetPosition());
                    spatial->GetLocalTransform().SetRotation(c.LocalTransform.GetRotation());
                    spatial->GetLocalTransform().SetScale(c.LocalTransform.GetScale());
                    if (!c.Parent.empty())
                    {
                        auto* parent = actor->FindComponent<SceneComponent>(c.Parent);
                        if (!parent || !spatial->SetParent(parent,ReparentMode::KeepLocal)) error(a.Id,c.Name,"parent","Missing, nonspatial or cyclic component parent");
                    }
                }
                else if (!c.Parent.empty()) error(a.Id,c.Name,"parent","Nonspatial components cannot attach");
                try { if (c.Configure) c.Configure(*component); }
                catch (const std::exception& e) { error(a.Id,c.Name,"configuration",e.what()); }
                catch (...) { error(a.Id,c.Name,"configuration","Unknown configuration exception"); }
            }
        }
        auto* cameraActor = candidate->FindActor(scene.CameraActor);
        auto* camera = cameraActor ? cameraActor->FindComponent<CameraComponent>(scene.CameraComponent) : nullptr;
        if (!camera) error(scene.CameraActor,scene.CameraComponent,"camera","Explicit camera reference must resolve to a CameraComponent");
        else result.Camera = camera->GetHandle<CameraComponent>();
        if (result.Diagnostics.empty()) candidate->Prepare(result.Diagnostics);
    }
    catch (const std::exception& e) { error({},{},{},e.what()); }
    catch (...) { error({},{},{},"Unknown scene construction exception"); }
    if (result.Diagnostics.empty()) result.Candidate = std::move(candidate);
    return result;
}
