#include "UnrealSceneAdapter.h"
#include "Maths.hpp"
#include "Quaternion.hpp"
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace
{
	using Matrix4f = CU::Matrix4f;
	using Vector3f = CU::Vector3f;
	Matrix4f MapAxes(const Matrix4f& source, float unitScale)
	{
		Matrix4f result{
			source(2,2), source(2,3), source(2,1), source(2,4),
			source(3,2), source(3,3), source(3,1), source(3,4),
			source(1,2), source(1,3), source(1,1), source(1,4),
			source(4,1) * unitScale, source(4,2) * unitScale, source(4,3) * unitScale, source(4,4)};
		return result;
	}
	Vector3f MapAxes(const Vector3f& source, float unitScale)
	{
		return {source.y * unitScale, source.z * unitScale, source.x * unitScale};
	}
	std::optional<TransformData> TransformFrom(const Matrix4f& source, float unitScale)
	{
		const auto mapped = MapAxes(source, unitScale); Vector3f scale, position; CU::Quaternion<float> rotation;
		if (!CU::DecomposeSRT(mapped, scale, rotation, position)) return std::nullopt;
		const auto radians = CU::YawPitchRollFromQuaternion(rotation);
		TransformData result; result.Position = position; result.Scale = scale;
		result.RotationDegrees = {CU::RadiansToDegrees(radians.x), CU::RadiansToDegrees(radians.y), CU::RadiansToDegrees(radians.z)};
		return result;
	}
	const BaseComponentData& BaseOf(const ImportedComponentData& value)
	{
		return std::visit([](const auto& component) -> const BaseComponentData& { return component; }, value);
	}
	ComponentData Common(const BaseComponentData& source, const TransformData& transform)
	{
		ComponentData result; 
		result.Name = source.Name; 
		result.Tags = source.Tags; 
		result.SourceParent = source.Parent; 
		result.Transform = transform; 
		return result;
	}
	PlaceholderComponentType Placeholder(UnrealComponentType type)
	{
		switch (type) 
		{ 
		case UnrealComponentType::Box: 
			return PlaceholderComponentType::Box; 
		case UnrealComponentType::Sphere: 
			return PlaceholderComponentType::Sphere;
		case UnrealComponentType::Capsule: 
			return PlaceholderComponentType::Capsule; 
		case UnrealComponentType::SpringArm: 
			return PlaceholderComponentType::SpringArm; 
		default: 
			return PlaceholderComponentType::Custom; 
		}
	}
}

SceneConversionResult UnrealSceneAdapter::Convert(const UnrealSceneData& source, const UnrealSceneAdapterConfig& config) const
{
	SceneConversionResult result; SceneData scene;
	if (!CU::IsFinite(config.UnitScale) || config.UnitScale <= 0) { result.Diagnostics.push_back({"Adapter", "UnitScale must be finite and positive"}); return result; }
	for (const auto& sourceActor : source.Actors)
	{
		ActorRecord actor; actor.Name = sourceActor.Name; actor.Archetype = sourceActor.Archetype; actor.Tags = sourceActor.Tags;
		auto actorTransform = TransformFrom(sourceActor.Transform, config.UnitScale);
		if (!actorTransform) { result.Diagnostics.push_back({sourceActor.Name, "actor transform cannot be decomposed"}); continue; }
		actor.Transform = *actorTransform;
		for (const auto& sourceRecord : sourceActor.Components)
		{
			const auto& base = BaseOf(sourceRecord); const std::string context = sourceActor.Name + "/" + base.Name;
			auto transform = TransformFrom(base.Transform, config.UnitScale);
			if (!transform) { result.Diagnostics.push_back({context, "component transform cannot be decomposed"}); continue; }
			std::visit([&](const auto& imported)
			{
				using T = std::decay_t<decltype(imported)>; const auto common = Common(base, *transform);
				if constexpr (std::is_same_v<T, ImportedMeshComponent>)
				{
					StaticMeshData mesh; mesh.Common = common; mesh.MeshName = imported.Mesh; mesh.ContentPath = imported.ContentPath; mesh.Mesh = AssetId{imported.ContentPath};
					for (const auto& importedMaterial : imported.Materials)
					{
						MaterialInstanceData material; material.Name = importedMaterial.Name; material.Parent = AssetId{importedMaterial.Parent.empty() ? importedMaterial.Name : importedMaterial.Parent};
						for (const auto& importedParameter : importedMaterial.Parameters)
						{
							MaterialParameterData parameter; parameter.Name = importedParameter.Name;
							if (const auto* scalar = std::get_if<float>(&importedParameter.Value)) parameter.Value = *scalar;
							else if (const auto* vector = std::get_if<CU::Vector4f>(&importedParameter.Value)) parameter.Value = *vector;
							else if (const auto* texture = std::get_if<TextureValue>(&importedParameter.Value)) parameter.Value = AssetId{texture->Path};
							else { result.Diagnostics.push_back({context + "/" + importedMaterial.Name + "/" + importedParameter.Name, "material parameter type has no runtime representation"}); continue; }
							material.Parameters.push_back(std::move(parameter));
						}
						mesh.Materials.push_back(std::move(material));
					}
					if (base.TypeID == UnrealComponentType::SkeletalMesh) { SkeletalMeshData skeletal; static_cast<StaticMeshData&>(skeletal) = std::move(mesh); actor.Components.push_back(std::move(skeletal)); }
					else actor.Components.push_back(std::move(mesh));
				}
				else if constexpr (std::is_same_v<T, ImportedSpotLightComponent>) { SpotLightData light; light.Common = common; light.Color = {imported.Color.x,imported.Color.y,imported.Color.z}; light.ColorAlpha = imported.Color.w; light.Intensity = imported.Intensity; light.Radius = imported.AttenuationRadius * config.UnitScale; light.FalloffExponent = imported.FalloffExponent; light.InnerConeDegrees = imported.InnerConeAngle; light.OuterConeDegrees = imported.OuterConeAngle; actor.Components.push_back(std::move(light)); }
				else if constexpr (std::is_same_v<T, ImportedPointLightComponent>) { PointLightData light; light.Common = common; light.Color = {imported.Color.x,imported.Color.y,imported.Color.z}; light.ColorAlpha = imported.Color.w; light.Intensity = imported.Intensity; light.Radius = imported.AttenuationRadius * config.UnitScale; light.FalloffExponent = imported.FalloffExponent; actor.Components.push_back(std::move(light)); }
				else if constexpr (std::is_same_v<T, ImportedLightComponent>) { DirectionalLightData light; light.Common = common; light.Color = {imported.Color.x,imported.Color.y,imported.Color.z}; light.ColorAlpha = imported.Color.w; light.Intensity = imported.Intensity; actor.Components.push_back(std::move(light)); }
				else if (base.TypeID == UnrealComponentType::SceneComponent && std::find(base.Tags.begin(), base.Tags.end(), "ActiveCamera") != base.Tags.end()) { CameraData component; component.Common = common; actor.Components.push_back(std::move(component)); }
				else if (base.TypeID == UnrealComponentType::SceneComponent) { SceneComponentData component; component.Common = common; actor.Components.push_back(std::move(component)); }
				else
				{
					PlaceholderComponentData component; component.Common = common; component.Type = Placeholder(base.TypeID);
					if constexpr (std::is_same_v<T, ImportedBoxComponent>) component.Properties = BoxPlaceholderData{MapAxes(imported.BoundsMaxima, config.UnitScale)};
					else if constexpr (std::is_same_v<T, ImportedSphereComponent>) component.Properties = SpherePlaceholderData{imported.Radius * config.UnitScale};
					else if constexpr (std::is_same_v<T, ImportedCapsuleComponent>) component.Properties = CapsulePlaceholderData{imported.Radius * config.UnitScale, imported.HalfHeight * config.UnitScale};
					else if constexpr (std::is_same_v<T, ImportedSpringArmComponent>) component.Properties = SpringArmPlaceholderData{MapAxes(imported.SocketOffset, config.UnitScale), imported.ArmLength * config.UnitScale};
					actor.Components.push_back(std::move(component));
				}
			}, sourceRecord);
		}
		scene.Actors.push_back(std::move(actor));
	}
	if (result.Diagnostics.empty()) result.Scene = std::move(scene);
	return result;
}
