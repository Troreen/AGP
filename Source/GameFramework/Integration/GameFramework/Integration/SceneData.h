#pragma once
#include "GameFramework/Transform.h"
#include "GameFramework/SceneDiagnostic.h"
#include "GameFramework/AssetRefs.h"
#include "Vector4.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Normalized engine-space input, independent of any importer-owned format.
struct ObjectAddress
{
    std::string ActorId;
    std::string ComponentId;
};

using PropertyValue = std::variant<bool, int64_t, double, std::string,
    CommonUtilities::Vector3f, CommonUtilities::Quaternion<float>, CommonUtilities::Vector4f,
    AssetId, std::vector<AssetId>, ObjectAddress>;
using PropertyMap = std::map<std::string, PropertyValue>;

struct ComponentRecord
{
    std::string Id;
    std::string Name;
    std::string Type;
    bool Enabled = true;
    std::optional<LocalPose> Pose;
    std::string Parent;
    PropertyMap Properties;
    SourceLocation Source;
};

struct ActorRecord
{
    std::string Id;
    std::string Name;
    std::string Parent;
    LocalPose Pose;
    bool Active = true;
    std::vector<ComponentRecord> Components;
    SourceLocation Source;
};

struct SceneData
{
    std::vector<ActorRecord> Actors;
    std::optional<ObjectAddress> ActiveCamera;
    bool RequireCamera = false;
    SourceLocation Source;
    std::string Version;
};
