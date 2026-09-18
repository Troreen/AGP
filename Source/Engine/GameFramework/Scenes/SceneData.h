#pragma once
#include "GameFramework/Scenes/AssetRefs.h"
#include "GameFramework/World/Transform.h"
#include "Vector2.hpp"
#include "Vector4.hpp"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <variant>
#include <vector>

struct ComponentData
{
	std::string Name;
	bool Enabled = true;
	TransformData Transform;
	std::vector<std::string> Tags;
	std::string SourceParent;
};

struct SceneComponentData { ComponentData Common; };
struct CameraData { ComponentData Common; float FieldOfView = 90, NearPlane = 1, FarPlane = 50000; };
struct MaterialParameterData
{
	std::string Name;
	std::variant<float, CommonUtilities::Vector4f, AssetId> Value;
};
struct MaterialInstanceData
{
	std::string Name;
	AssetId Parent;
	std::vector<MaterialParameterData> Parameters;
};
struct StaticMeshData
{
	ComponentData Common;
	std::string MeshName;
	std::string ContentPath;
	AssetId Mesh;
	std::vector<MaterialInstanceData> Materials;
	bool Visible = true;
};
struct SkeletalMeshData : StaticMeshData
{
	std::string InitialAnimation;
	std::string PartialRoot;
	bool Loop = true;
};
struct DirectionalLightData { ComponentData Common; CommonUtilities::Vector3f Color{1,1,1}; float ColorAlpha = 1; float Intensity = 1; };
struct PointLightData : DirectionalLightData { float Radius = 1000; float FalloffExponent = 0; };
struct SpotLightData : PointLightData { float InnerConeDegrees = 20; float OuterConeDegrees = 35; };

enum class PlaceholderComponentType { Box, Sphere, Capsule, SpringArm, Custom };
struct BoxPlaceholderData { CommonUtilities::Vector3f BoundsMaxima; };
struct SpherePlaceholderData { float Radius = 0.0f; };
struct CapsulePlaceholderData { float Radius = 0.0f; float HalfHeight = 0.0f; };
struct SpringArmPlaceholderData { CommonUtilities::Vector3f SocketOffset; float ArmLength = 0.0f; };
using PlaceholderProperties = std::variant<std::monostate, BoxPlaceholderData, SpherePlaceholderData,
	CapsulePlaceholderData, SpringArmPlaceholderData>;
struct PlaceholderComponentData
{
	ComponentData Common;
	PlaceholderComponentType Type = PlaceholderComponentType::Custom;
	PlaceholderProperties Properties;
};

using ComponentRecord = std::variant<SceneComponentData, CameraData, StaticMeshData, SkeletalMeshData, DirectionalLightData,
	PointLightData, SpotLightData, PlaceholderComponentData>;

struct ActorRecord
{
	std::string Name;
	std::string Archetype;
	std::vector<std::string> Tags;
	TransformData Transform;
	bool Active = true;
	std::vector<ComponentRecord> Components;
};

struct SceneData { std::vector<ActorRecord> Actors; };

struct SceneLoadContext
{
	const std::filesystem::path& ContentRoot;
	CommonUtilities::Vector2u ClientSize;
	AssetLibrary& Assets;
};

using SceneSource = std::function<SceneData(const std::string& name, SceneLoadContext& context)>;
