#pragma once

#include <Matrix.hpp>
#include <Vector.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Integration notes:
// - These are temporary, source-shaped records. UnrealSceneAdapter owns axis mapping,
//   unit conversion and conversion into the runtime SceneData types.
// - ImportScene returns diagnostics so a malformed file is not mistaken for an empty scene.
// - Materials remain ordered children of mesh components; they are not component records.
// - The component variant is closed so unknown exporter TypeIDs cannot be silently ignored.

enum class UnrealComponentType : int64_t
{
	Custom = -1,
	SceneComponent,
	StaticMesh,
	SkeletalMesh,
	PointLight,
	SpotLight,
	DirectionalLight,
	Box,
	Sphere,
	Capsule,
	SpringArm
};

enum class MaterialType : int64_t
{
	Scalar,
	Vector3f,
	Vector3d,
	Texture,
	TextureCollection,
	Font,
	RunTimeVirtualTexture,
	SparseVolumeTexture,
	StaticSwitch,
	ParameterCollection
};

struct ImportDiagnostic
{
	std::string Context;
	std::string Message;
};

struct BaseComponentData
{
	std::string Name;
	UnrealComponentType TypeID = UnrealComponentType::Custom;
	std::string Parent;
	std::vector<std::string> Tags;
	CommonUtilities::Matrix4f Transform;
};

struct TextureValue
{
	std::string Name;
	std::string Path;
};

using ImportedMaterialValue = std::variant<float, CommonUtilities::Vector4f, TextureValue>;

struct ImportedMaterialParameter
{
	std::string Name;
	MaterialType Type = MaterialType::Scalar;
	ImportedMaterialValue Value = 0.0f;
};

struct ImportedMaterial
{
	std::string Name;
	std::string Parent;
	std::vector<ImportedMaterialParameter> Parameters;
};

// One source record serves both static and skeletal mesh TypeIDs; the adapter emits
// their distinct strongly typed runtime descriptions.
struct ImportedMeshComponent : public BaseComponentData
{
	std::string Mesh;
	std::string ContentPath;
	std::vector<ImportedMaterial> Materials;
};

struct ImportedLightComponent : public BaseComponentData
{
	CommonUtilities::Vector4f Color;
	float Intensity = 0.0f;
};

struct ImportedPointLightComponent : public ImportedLightComponent
{
	float FalloffExponent = 0.0f;
	float AttenuationRadius = 0.0f;
};

struct ImportedSpotLightComponent : public ImportedPointLightComponent
{
	float InnerConeAngle = 0.0f;
	float OuterConeAngle = 0.0f;
};

// These source-specific placeholder records were added after the first integration
// test. They keep the other team's exported values available to the typed adapter.
struct ImportedBoxComponent : public BaseComponentData
{
	CommonUtilities::Vector3f BoundsMaxima;
};

struct ImportedSphereComponent : public BaseComponentData
{
	float Radius = 0.0f;
};

struct ImportedCapsuleComponent : public ImportedSphereComponent
{
	float HalfHeight = 0.0f;
};

struct ImportedSpringArmComponent : public BaseComponentData
{
	CommonUtilities::Vector3f SocketOffset;
	float ArmLength = 0.0f;
};

using ImportedComponentData = std::variant<
	BaseComponentData,
	ImportedMeshComponent,
	ImportedLightComponent,
	ImportedPointLightComponent,
	ImportedSpotLightComponent,
	ImportedBoxComponent,
	ImportedSphereComponent,
	ImportedCapsuleComponent,
	ImportedSpringArmComponent>;

struct UnrealActorData
{
	std::string Name;
	std::string Archetype;
	std::vector<std::string> Tags;
	CommonUtilities::Matrix4f Transform;
	std::vector<ImportedComponentData> Components;
};

struct UnrealSceneData
{
	std::vector<UnrealActorData> Actors;
};

struct UnrealImportResult
{
	std::optional<UnrealSceneData> Data;
	std::vector<ImportDiagnostic> Diagnostics;

	explicit operator bool() const { return Data.has_value() && Diagnostics.empty(); }
};
