#pragma once
#include "GameFramework/Components/SceneComponent.h"
#include <functional>
#include <string>
#include <vector>

// In-memory authoring API, deliberately independent of serialization. A future
// importer adapter translates its payloads into these creation/configuration steps.
struct ComponentDescription
{
    std::string Name;
    std::string Type;
    std::string Parent;
    CommonUtilities::Transform LocalTransform;
    bool Enabled = true;
    std::function<void(Component&)> Configure;
    template<class T, class ConfigureFunction>
    static ComponentDescription Make(std::string name, std::string type, ConfigureFunction configure)
    {
        ComponentDescription description;
        description.Name = std::move(name); description.Type = std::move(type);
        description.Configure = [configure = std::move(configure)](Component& component) { configure(dynamic_cast<T&>(component)); };
        return description;
    }
};
struct ActorDescription
{
    // This first description uses a unique string identity as the runtime actor name.
    std::string Id;
    std::string Parent;
    CommonUtilities::Transform LocalTransform;
    bool Active = true;
    std::vector<ComponentDescription> Components;
};
struct SceneDescription
{
    std::vector<ActorDescription> Actors;
    std::string CameraActor;
    std::string CameraComponent;
};
