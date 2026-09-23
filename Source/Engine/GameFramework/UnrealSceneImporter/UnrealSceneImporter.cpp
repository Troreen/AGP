#include "UnrealSceneImporter.h"
#include "UnrealSceneStructs.h"
#include "Maths.hpp"
#include "Quaternion.hpp"
#include <SimdJson/simdjson.h>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace UnrealScene
{
	void ReadCommonComponent(simdjson::dom::element component, BaseComponentData& outComponent, UnrealComponentType type)
	{
		outComponent.name = component["Name"];
		outComponent.parent = component["Parent"];
		outComponent.typeID = type;
		for (const auto& tag : component["Tags"])
		{
			outComponent.tags.emplace_back(tag.get_c_str().value());
		}

		const auto& matrix = component["Transform"].get_array().value();
		outComponent.transform = {
		    static_cast<float>(matrix.at(5).get_double().value()),  
			static_cast<float>(matrix.at(6).get_double().value()),
		    static_cast<float>(matrix.at(4).get_double().value()),  
			static_cast<float>(matrix.at(3).get_double().value()),
		    static_cast<float>(matrix.at(9).get_double().value()),  
			static_cast<float>(matrix.at(10).get_double().value()),
		    static_cast<float>(matrix.at(8).get_double().value()),  
			static_cast<float>(matrix.at(7).get_double().value()),
		    static_cast<float>(matrix.at(1).get_double().value()),  
			static_cast<float>(matrix.at(2).get_double().value()),
		    static_cast<float>(matrix.at(0).get_double().value()),  
			static_cast<float>(matrix.at(11).get_double().value()),
		    static_cast<float>(matrix.at(13).get_double().value()), 
			static_cast<float>(matrix.at(14).get_double().value()),
		    static_cast<float>(matrix.at(12).get_double().value()), 
			static_cast<float>(matrix.at(15).get_double().value())};
	}

	void ReadCommonLightComponent(simdjson::dom::element component, CommonLightComponentData& outComponent, UnrealComponentType type)
	{
		ReadCommonComponent(component, outComponent, type);
		outComponent.intensity = static_cast<float>(component["Intensity"].get_double().value());
		const auto& color = component["Color"].get_array().value();
		outComponent.color = {
		    static_cast<float>(color.at(0).get_double().value()),
		    static_cast<float>(color.at(1).get_double().value()),
		    static_cast<float>(color.at(2).get_double().value()),
			static_cast<float>(color.at(3).get_double().value())
		};
	}

UnrealSceneData ImportScene(std::filesystem::path aJSONPath)
{
	simdjson::padded_string json = simdjson::padded_string::load(aJSONPath.c_str());
	simdjson::dom::parser parser;
	simdjson::dom::object root;
	const auto& error = parser.parse(json).get(root);

	if (error)
	{
		return {};
	}

	UnrealSceneData unrealData = {};
	unrealData.parsed = true;

	for (const auto& actor : root["Actors"])
	{
		UnrealActorData actorData = {};
		actorData.name = actor["Name"];
		actorData.archetype = actor["Archetype"];
		for (const auto& tag : actor["Tags"])
		{
			std::string actorTag(tag.get_c_str());
			actorData.tags.emplace_back(actorTag);
		}

		{
			const auto& matrix = actor["Transform"].get_array().value();
			actorData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
				                   static_cast<float>(matrix.at(6).get_double().value()),
				                   static_cast<float>(matrix.at(4).get_double().value()),
				                   static_cast<float>(matrix.at(3).get_double().value()),
				                   static_cast<float>(matrix.at(9).get_double().value()),
				                   static_cast<float>(matrix.at(10).get_double().value()),
				                   static_cast<float>(matrix.at(8).get_double().value()),
				                   static_cast<float>(matrix.at(7).get_double().value()),
				                   static_cast<float>(matrix.at(1).get_double().value()),
				                   static_cast<float>(matrix.at(2).get_double().value()),
				                   static_cast<float>(matrix.at(0).get_double().value()),
				                   static_cast<float>(matrix.at(11).get_double().value()),
				                   static_cast<float>(matrix.at(13).get_double().value()),
				                   static_cast<float>(matrix.at(14).get_double().value()),
				                   static_cast<float>(matrix.at(12).get_double().value()),
				                   static_cast<float>(matrix.at(15).get_double().value())

			};
		}

		for (const auto& component : actor["Components"])
		{
			const UnrealComponentType type = static_cast<UnrealComponentType>(component["TypeID"].get_int64().value());
			switch (type)
			{
				case UnrealComponentType::Custom:
				case UnrealComponentType::SceneComponent:
				{
					BaseComponentData componentData = {};
					ReadCommonComponent(component, componentData, type);
					actorData.components.push_back(std::move(componentData));
				}
				break;
				case UnrealComponentType::StaticMesh:
				case UnrealComponentType::SkeletalMesh:
				{
					StaticMeshComponentData componentData = {};
					ReadCommonComponent(component, componentData, type);

					componentData.mesh = component["Mesh"];
					componentData.contentPath = component["ContentPath"];

					for (const auto& material : component["Materials"])
					{

						MaterialData materialData;
						materialData.name = material["Name"];
						materialData.parent = material["Parent"].has_value() ? material["Parent"].get_c_str().value() : "";

						for (const auto& parameter : material["Parameters"].get_array().value())
						{
							MaterialParameterData parameterData;
							parameterData.name = parameter["Name"];
							parameterData.type = static_cast<MaterialType>(parameter["Type"].get_int64().value());

							switch (parameterData.type)
							{
								case MaterialType::Scalar:
								{
									float scalar = 0;

									scalar = static_cast<float>(parameter["Value"].get_double());
									parameterData.value = scalar;
								}
								break;
								case MaterialType::Vector3f:
								{
									CommonUtilities::Vector4f vector;

									const auto& value = parameter["Value"].get_array().value();

									vector.x = static_cast<float>(value.at(0).get_double());
									vector.y = static_cast<float>(value.at(1).get_double());
									vector.z = static_cast<float>(value.at(2).get_double());
									vector.w = static_cast<float>(value.at(3).get_double());

									parameterData.value = vector;
								}

								break;
								case MaterialType::Texture:
								{
									TextureValue textureValue;
									const auto& value = parameter["Value"].get_object().value();
									textureValue.name = value["Name"].get_string().value();
									textureValue.path = value["Path"].get_string().value();

									parameterData.value = textureValue;
								}
								break;
								default:
									continue;
							}
							materialData.parameters.emplace_back(parameterData);
							}
							componentData.materials.emplace_back(materialData);
					}

					actorData.components.emplace_back(componentData);
				}
				break;
				case UnrealComponentType::PointLight:
				{
					PointLightComponentData componentData = {};
					ReadCommonLightComponent(component, componentData, UnrealComponentType::PointLight);

					componentData.attenuationRadius = static_cast<float>(component["AttenuationRadius"].get_double());
					componentData.falloffExponent = static_cast<float>(component["FalloffExponent"].get_double());
					actorData.components.push_back(std::move(componentData));
				}

				break;
				case UnrealComponentType::SpotLight:
				{
					SpotLightComponentData componentData = {};
					ReadCommonLightComponent(component, componentData, UnrealComponentType::SpotLight);

					componentData.attenuationRadius = static_cast<float>(component["AttenuationRadius"].get_double());
					componentData.outerConeAngle = static_cast<float>(component["OuterConeAngle"].get_double());
					componentData.innerConeAngle = static_cast<float>(component["InnerConeAngle"].get_double());

					componentData.falloffExponent = static_cast<float>(component["FalloffExponent"].get_double());
					actorData.components.push_back(std::move(componentData));
				}
				break;
				case UnrealComponentType::DirectionalLight:
				{
					CommonLightComponentData componentData = {};
					ReadCommonLightComponent(component, componentData, UnrealComponentType::DirectionalLight);

					actorData.components.push_back(std::move(componentData));
				}

				break;
				case UnrealComponentType::Box:
				{
					BoxComponentData componentData = {};

					ReadCommonComponent(component, componentData, UnrealComponentType::Box);

					CommonUtilities::Vector3f bounds;
					const auto& array = component["Bounds"].get_array().value();
					bounds.x = static_cast<float>(array.at(1).get_double().value());
					bounds.y = static_cast<float>(array.at(2).get_double().value());
					bounds.z = static_cast<float>(array.at(0).get_double().value());
					componentData.boundsMaxima = bounds;

					actorData.components.push_back(std::move(componentData));
				}
				break;
				case UnrealComponentType::Sphere:
				{
					BaseSphericalComponentData componentData = {};
					ReadCommonComponent(component, componentData, UnrealComponentType::Sphere);

					componentData.radius = static_cast<float>(component["Radius"].get_double().value());
					actorData.components.push_back(std::move(componentData));
				}
				break;
				case UnrealComponentType::Capsule:
				{

					CapsuleComponentData componentData = {};
					ReadCommonComponent(component, componentData, UnrealComponentType::Capsule);
					componentData.radius = static_cast<float>(component["Radius"].get_double().value());
					componentData.halfHeight = static_cast<float>(component["HalfHeight"].get_double().value());
					actorData.components.push_back(std::move(componentData));
				}
				break;
				case UnrealComponentType::SpringArm:
				{
					SpringArmComponentData componentData = {};
					ReadCommonComponent(component, componentData, UnrealComponentType::SpringArm);
					const auto socketOffset = component["SocketOffset"].get_object().value();
					componentData.socketOffset =
					{
						static_cast<float>(socketOffset["y"].get_double().value()),
						static_cast<float>(socketOffset["z"].get_double().value()),
						static_cast<float>(socketOffset["x"].get_double().value())
					};
					componentData.armLength = static_cast<float>(component["ArmLength"].get_double().value());
					actorData.components.push_back(std::move(componentData));
				}
				break;
				default:
					throw std::runtime_error("Unknown component TypeID: " +
					                         std::to_string(component["TypeID"].get_int64().value()));
			}
		}
		unrealData.actors.emplace_back(actorData);
	}

	return unrealData;
}

}

namespace
{
	constexpr float UnrealUnitScale = 1.0f;
	using UnrealScene::UnrealComponentType;

	std::optional<TransformData> ConvertTransform(const CommonUtilities::Matrix4f& source)
	{
		CommonUtilities::Vector3f scale;
		CommonUtilities::Vector3f position;
		CommonUtilities::Quaternion<float> rotation;
		if (!CommonUtilities::DecomposeSRT(source, scale, rotation, position))
		{
			return std::nullopt;
		}

		const CommonUtilities::Vector3f rotationRadians = CommonUtilities::YawPitchRollFromQuaternion(rotation);
		TransformData transform;
		transform.Position = position * UnrealUnitScale;
		transform.Scale = scale;
		transform.RotationDegrees = {
			CommonUtilities::RadiansToDegrees(rotationRadians.x),
			CommonUtilities::RadiansToDegrees(rotationRadians.y),
			CommonUtilities::RadiansToDegrees(rotationRadians.z)};
		return transform;
	}

	const UnrealScene::BaseComponentData& GetBaseComponent(const UnrealScene::ComponentData& component)
	{
		return std::visit([](const auto& value) -> const UnrealScene::BaseComponentData& { return value; }, component);
	}

	ComponentData ConvertCommonData(const UnrealScene::BaseComponentData& source, const TransformData& transform)
	{
		ComponentData component;
		component.Name = source.name;
		component.SourceParent = source.parent;
		component.Tags = source.tags;
		component.Transform = transform;
		return component;
	}

	PlaceholderComponentType ConvertPlaceholderType(UnrealComponentType type)
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

	struct RuntimeConversionResult
	{
		std::optional<SceneData> Scene;
		std::vector<ImportDiagnostic> Diagnostics;
	};

	RuntimeConversionResult ConvertRuntimeScene(const UnrealScene::UnrealSceneData& source)
	{
		RuntimeConversionResult result;
		SceneData scene;
		for (const UnrealScene::UnrealActorData& sourceActor : source.actors)
		{
			const std::optional<TransformData> actorTransform = ConvertTransform(sourceActor.transform);
			if (!actorTransform)
			{
				result.Diagnostics.push_back({sourceActor.name, "actor transform cannot be decomposed"});
				continue;
			}

			ActorRecord actor;
			actor.Name = sourceActor.name;
			actor.Archetype = sourceActor.archetype;
			actor.Tags = sourceActor.tags;
			actor.Transform = *actorTransform;

			for (const UnrealScene::ComponentData& sourceComponent : sourceActor.components)
			{
				const UnrealScene::BaseComponentData& base = GetBaseComponent(sourceComponent);
				const std::string context = sourceActor.name + "/" + base.name;
				const std::optional<TransformData> componentTransform = ConvertTransform(base.transform);
				if (!componentTransform)
				{
					result.Diagnostics.push_back({context, "component transform cannot be decomposed"});
					continue;
				}

				std::visit([&](const auto& imported)
				{
					using SourceType = std::decay_t<decltype(imported)>;
					const ComponentData common = ConvertCommonData(base, *componentTransform);
					if constexpr (std::is_same_v<SourceType, UnrealScene::StaticMeshComponentData>)
					{
						StaticMeshData mesh;
						mesh.Common = common;
						mesh.MeshName = imported.mesh;
						mesh.ContentPath = imported.contentPath;
						for (const UnrealScene::MaterialData& sourceMaterial : imported.materials)
						{
							MaterialInstanceData material;
							material.Name = sourceMaterial.name;
							for (const UnrealScene::MaterialParameterData& sourceParameter : sourceMaterial.parameters)
							{
								MaterialParameterData parameter;
								parameter.Name = sourceParameter.name;
								if (const float* scalar = std::get_if<float>(&sourceParameter.value))
								{
									parameter.Value = *scalar;
								}
								else if (const CommonUtilities::Vector4f* vector =
								             std::get_if<CommonUtilities::Vector4f>(&sourceParameter.value))
								{
									parameter.Value = *vector;
								}
								else if (const UnrealScene::TextureValue* texture =
								             std::get_if<UnrealScene::TextureValue>(&sourceParameter.value))
								{
									parameter.Value = texture->path;
								}
								material.Parameters.push_back(std::move(parameter));
							}
							mesh.Materials.push_back(std::move(material));
						}
						if (base.typeID == UnrealComponentType::SkeletalMesh)
						{
							SkeletalMeshData skeletalMesh;
							static_cast<StaticMeshData&>(skeletalMesh) = std::move(mesh);
							actor.Components.push_back(std::move(skeletalMesh));
						}
						else
						{
							actor.Components.push_back(std::move(mesh));
						}
					}
					else if constexpr (std::is_same_v<SourceType, UnrealScene::SpotLightComponentData>)
					{
						SpotLightData light;
						light.Common = common;
						light.Color = {imported.color.x, imported.color.y, imported.color.z};
						light.ColorAlpha = imported.color.w;
						light.Intensity = imported.intensity;
						light.Radius = imported.attenuationRadius * UnrealUnitScale;
						light.FalloffExponent = imported.falloffExponent;
						light.InnerConeDegrees = imported.innerConeAngle;
						light.OuterConeDegrees = imported.outerConeAngle;
						actor.Components.push_back(std::move(light));
					}
					else if constexpr (std::is_same_v<SourceType, UnrealScene::PointLightComponentData>)
					{
						PointLightData light;
						light.Common = common;
						light.Color = {imported.color.x, imported.color.y, imported.color.z};
						light.ColorAlpha = imported.color.w;
						light.Intensity = imported.intensity;
						light.Radius = imported.attenuationRadius * UnrealUnitScale;
						light.FalloffExponent = imported.falloffExponent;
						actor.Components.push_back(std::move(light));
					}
					else if constexpr (std::is_same_v<SourceType, UnrealScene::CommonLightComponentData>)
					{
						DirectionalLightData light;
						light.Common = common;
						light.Color = {imported.color.x, imported.color.y, imported.color.z};
						light.ColorAlpha = imported.color.w;
						light.Intensity = imported.intensity;
						actor.Components.push_back(std::move(light));
					}
					else if (base.typeID == UnrealComponentType::SceneComponent &&
					         std::find(base.tags.begin(), base.tags.end(), "ActiveCamera") != base.tags.end())
					{
						actor.Components.push_back(CameraData{common});
					}
					else if (base.typeID == UnrealComponentType::SceneComponent)
					{
						actor.Components.push_back(SceneComponentData{common});
					}
					else
					{
						PlaceholderComponentData placeholder;
						placeholder.Common = common;
						placeholder.Type = ConvertPlaceholderType(base.typeID);
						if constexpr (std::is_same_v<SourceType, UnrealScene::BoxComponentData>)
						{
							placeholder.Properties = BoxPlaceholderData{imported.boundsMaxima * UnrealUnitScale};
						}
						else if constexpr (std::is_same_v<SourceType, UnrealScene::BaseSphericalComponentData>)
						{
							placeholder.Properties = SpherePlaceholderData{imported.radius * UnrealUnitScale};
						}
						else if constexpr (std::is_same_v<SourceType, UnrealScene::CapsuleComponentData>)
						{
							placeholder.Properties = CapsulePlaceholderData{
								imported.radius * UnrealUnitScale, imported.halfHeight * UnrealUnitScale};
						}
						else if constexpr (std::is_same_v<SourceType, UnrealScene::SpringArmComponentData>)
						{
							placeholder.Properties = SpringArmPlaceholderData{
								imported.socketOffset * UnrealUnitScale, imported.armLength * UnrealUnitScale};
						}
						actor.Components.push_back(std::move(placeholder));
					}
				}, sourceComponent);
			}
			scene.Actors.push_back(std::move(actor));
		}

		if (result.Diagnostics.empty())
		{
			result.Scene = std::move(scene);
		}
		return result;
	}
}

UnrealImportResult UnrealSceneImporter::ImportScene(const std::filesystem::path& jsonPath) const
{
	UnrealImportResult result;
	try
	{
		const UnrealScene::UnrealSceneData importedScene = UnrealScene::ImportScene(jsonPath);
		if (!importedScene.parsed)
		{
			result.Diagnostics.push_back({jsonPath.string(), "could not parse scene JSON"});
			return result;
		}
		RuntimeConversionResult converted = ConvertRuntimeScene(importedScene);
		result.Diagnostics = std::move(converted.Diagnostics);
		result.Data = std::move(converted.Scene);
	}
	catch (const std::exception& error)
	{
		result.Diagnostics.push_back({jsonPath.string(), error.what()});
	}
	return result;
}
