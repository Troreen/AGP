#pragma once

#include "UnrealSceneStructs.h"

// Compatibility DTOs for the Perforce importer. The parser owns this source-shaped
// representation; UnrealSceneImporter.cpp translates it to the framework records.
namespace PerforceScene
{
	using ::MaterialType;
	using ::UnrealComponentType;

	struct TextureValue
	{
		std::string name;
		std::string path;
	};

	using MaterialValue = std::variant<float, CommonUtilities::Vector4f, TextureValue>;

	struct MaterialParameterData
	{
		std::string name;
		MaterialType type = MaterialType::Scalar;
		MaterialValue value = 0.0f;
	};

	struct MaterialData
	{
		std::string name;
		std::string parent;
		std::vector<MaterialParameterData> parameters;
	};

	struct BaseComponentData
	{
		std::string name;
		UnrealComponentType typeID = UnrealComponentType::Custom;
		std::string parent;
		std::vector<std::string> tags;
		CommonUtilities::Matrix4f transform;
	};

	struct StaticMeshComponentData : BaseComponentData
	{
		std::string mesh;
		std::string contentPath;
		std::vector<MaterialData> materials;
	};

	struct CommonLightComponentData : BaseComponentData
	{
		CommonUtilities::Vector4f color;
		float intensity = 0.0f;
	};

	struct PointLightComponentData : CommonLightComponentData
	{
		float falloffExponent = 0.0f;
		float attenuationRadius = 0.0f;
	};

	struct SpotLightComponentData : PointLightComponentData
	{
		float innerConeAngle = 0.0f;
		float outerConeAngle = 0.0f;
	};

	struct BoxComponentData : BaseComponentData
	{
		CommonUtilities::Vector3f boundsMaxima;
	};

	struct BaseSphericalComponentData : BaseComponentData
	{
		float radius = 0.0f;
	};

	struct CapsuleComponentData : BaseSphericalComponentData
	{
		float halfHeight = 0.0f;
	};

	struct SpringArmComponentData : BaseComponentData
	{
		CommonUtilities::Vector3f socketOffset;
		float armLength = 0.0f;
	};

	using ComponentData = std::variant<
		BaseComponentData,
		StaticMeshComponentData,
		CommonLightComponentData,
		PointLightComponentData,
		SpotLightComponentData,
		BoxComponentData,
		BaseSphericalComponentData,
		CapsuleComponentData,
		SpringArmComponentData>;

	struct UnrealActorData
	{
		std::string name;
		std::string archetype;
		std::vector<std::string> tags;
		CommonUtilities::Matrix4f transform;
		std::vector<ComponentData> components;
	};

	struct UnrealSceneData
	{
		std::vector<UnrealActorData> actors;
		bool parsed = false;
	};
}
