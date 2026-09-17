#pragma once
#include "GameFramework/World/Transform.h"
#include "GameFramework/Scenes/AssetRefs.h"
#include <map>
#include <variant>
#include <vector>
#include <optional>
#include <filesystem>
#include <functional>
#include <cstdint>
#include "Vector2.hpp"

// Plain engine-facing descriptions. A future importer only needs to produce these.
using PropertyValue = std::variant<bool, int64_t, double, std::string, CommonUtilities::Vector3f, AssetId, std::vector<AssetId>>;
using PropertyMap = std::map<std::string, PropertyValue>;

struct ComponentRecord
{
	std::string Name;
	std::string Type;
	bool Enabled = true;
	std::optional<LocalPose> Pose;
	PropertyMap Properties;
};

struct ActorRecord
{
	std::string Name;
	LocalPose Pose;
	bool Active = true;
	std::vector<ComponentRecord> Components;
};

struct SceneData
{
	std::vector<ActorRecord> Actors;
	std::string ActiveCameraActor;
};

struct SceneLoadContext
{
	const std::filesystem::path& ContentRoot;
	CommonUtilities::Vector2u ClientSize;
	AssetLibrary& Assets;
};

// Synchronous. Throw an exception with a useful message on a load error.
using SceneSource = std::function<SceneData(const std::string& name, SceneLoadContext& context)>;
