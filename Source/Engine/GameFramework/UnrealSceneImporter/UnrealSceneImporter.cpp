#include "UnrealSceneImporter.h"
#include "PerforceSceneStructs.h"
#include "Maths.hpp"
#include "Quaternion.hpp"
#include <SimdJson/simdjson.h>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace PerforceScene
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
		    static_cast<float>(matrix.at(5).get_double().value()),  static_cast<float>(matrix.at(6).get_double().value()),
		    static_cast<float>(matrix.at(4).get_double().value()),  static_cast<float>(matrix.at(7).get_double().value()),
		    static_cast<float>(matrix.at(9).get_double().value()),  static_cast<float>(matrix.at(10).get_double().value()),
		    static_cast<float>(matrix.at(8).get_double().value()),  static_cast<float>(matrix.at(7).get_double().value()),
		    static_cast<float>(matrix.at(1).get_double().value()),  static_cast<float>(matrix.at(2).get_double().value()),
		    static_cast<float>(matrix.at(0).get_double().value()),  static_cast<float>(matrix.at(11).get_double().value()),
		    static_cast<float>(matrix.at(13).get_double().value()), static_cast<float>(matrix.at(14).get_double().value()),
		    static_cast<float>(matrix.at(12).get_double().value()), static_cast<float>(matrix.at(15).get_double().value())};
	}

UnrealSceneData ImportScene(std::filesystem::path aJSONPath)
{
	simdjson::padded_string json = simdjson::padded_string::load(aJSONPath.c_str());
	simdjson::dom::parser parser;
	simdjson::dom::object root;
	const auto& error = parser.parse(json).get(root);

	// TODO: create structs with level data
	//std::cout << root["Actors"].get_array().size() << std::endl;

	if (error)
	{
		return {};
	}

	UnrealSceneData unrealData = {};
	unrealData.parsed = true;

	for (const auto& actor : root["Actors"])
	{
		UnrealActorData actorData = {};
		//std::cout << actor["Name"].get_c_str() << std::endl;
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
				                   static_cast<float>(matrix.at(7).get_double().value()),
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
			switch (static_cast<UnrealComponentType>(component["TypeID"].get_int64().value()))
			{
				case UnrealComponentType::Custom:
				case UnrealComponentType::SceneComponent:
				{
					BaseComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = static_cast<UnrealComponentType>(component["TypeID"].get_int64().value());
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag(tag.get_c_str());
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
					                           static_cast<float>(matrix.at(6).get_double().value()),
					                           static_cast<float>(matrix.at(4).get_double().value()),
					                           static_cast<float>(matrix.at(7).get_double().value()),
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
					actorData.components.emplace_back(componentData);
				}
				break;
				case UnrealComponentType::StaticMesh:
				{
					StaticMeshComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::StaticMesh;
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
					                           static_cast<float>(matrix.at(6).get_double().value()),
					                           static_cast<float>(matrix.at(4).get_double().value()),
					                           static_cast<float>(matrix.at(7).get_double().value()),
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

					componentData.mesh = component["Mesh"];
					componentData.contentPath = component["ContentPath"];

					for (const auto& material : component["Materials"])
					{

						MaterialData& materialData = componentData.materials.emplace_back();
						materialData.name = material["Name"];
						materialData.parent = material["Parent"].has_value() ? material["Parent"].get_c_str().value() : "";

						for (const auto& parameter : material["Parameters"].get_array().value())
						{
							MaterialParameterData parameterData = materialData.parameters.emplace_back();
							parameterData.name = parameter["Name"];
							parameterData.type = static_cast<MaterialType>(parameter["Type"].get_int64().value());

							switch (parameterData.type)
							{
								case MaterialType::Scalar:
								{
									float scalar = 0;

									scalar = static_cast<float>(parameter["Value"].get_double());
									parameterData.value = scalar;
									materialData.parameters.emplace_back(parameterData);
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
									materialData.parameters.emplace_back(parameterData);
								}

								break;
								case MaterialType::Texture:
								{
									TextureValue textureValue;
									const auto& value = parameter["Value"].get_object().value();
									textureValue.name = value["Name"].get_string().value();
									textureValue.path = value["Path"].get_string().value();

									parameterData.value = textureValue;
									materialData.parameters.emplace_back(parameterData);
								}
								break;
								default:
									break;
							}
						}
						componentData.materials.emplace_back(materialData);
					}

					actorData.components.emplace_back(componentData);
				}
				break;
				case UnrealComponentType::SkeletalMesh:
					//If we want
				{
					StaticMeshComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::SkeletalMesh;
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
						                       static_cast<float>(matrix.at(6).get_double().value()),
						                       static_cast<float>(matrix.at(4).get_double().value()),
						                       static_cast<float>(matrix.at(7).get_double().value()),
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

					componentData.mesh = component["Mesh"];
					componentData.contentPath = component["ContentPath"];

					for (const auto& material : component["Materials"])
					{

						MaterialData& materialData = componentData.materials.emplace_back();
						materialData.name = material["Name"];
						materialData.parent = material["Parent"].has_value() ? material["Parent"].get_c_str().value() : "";

						for (const auto& parameter : material["Parameters"].get_array().value())
						{
							MaterialParameterData parameterData = materialData.parameters.emplace_back();
							parameterData.name = parameter["Name"];
							parameterData.type = static_cast<MaterialType>(parameter["Type"].get_int64().value());

							switch (parameterData.type)
							{
								case MaterialType::Scalar:
								{
									float scalar = 0;

									scalar = static_cast<float>(parameter["Value"].get_double());
									parameterData.value = scalar;
									materialData.parameters.emplace_back(parameterData);
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
									materialData.parameters.emplace_back(parameterData);
								}

								break;
								case MaterialType::Texture:
								{
									TextureValue textureValue;
									const auto& value = parameter["Value"].get_object().value();
									textureValue.name = value["Name"].get_string().value();
									textureValue.path = value["Path"].get_string().value();

									parameterData.value = textureValue;
									materialData.parameters.emplace_back(parameterData);
								}
								break;
								default:
									break;
							}
						}
						componentData.materials.emplace_back(materialData);
					}

					actorData.components.emplace_back(componentData);
				}
				break;
				case UnrealComponentType::PointLight:
				{
					PointLightComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];


					componentData.typeID = UnrealComponentType::PointLight;
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
					                           static_cast<float>(matrix.at(6).get_double().value()),
					                           static_cast<float>(matrix.at(4).get_double().value()),
					                           static_cast<float>(matrix.at(7).get_double().value()),
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

					CommonUtilities::Vector4f vector;
					const auto& array = component["Color"].get_array().value();
					vector.x = static_cast<float>(array.at(0).get_double().value());
					vector.y = static_cast<float>(array.at(1).get_double().value());
					vector.z = static_cast<float>(array.at(2).get_double().value());
					vector.w = static_cast<float>(array.at(3).get_double().value());

					componentData.color = vector;
					componentData.intensity = static_cast<float>(component["Intensity"].get_double());

					componentData.attenuationRadius = static_cast<float>(component["AttenuationRadius"].get_double());
					componentData.falloffExponent = static_cast<float>(component["FalloffExponent"].get_double());
					actorData.components.emplace_back(componentData);
				}

				break;
				case UnrealComponentType::SpotLight:
				{
					SpotLightComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];


					componentData.typeID = UnrealComponentType::SpotLight;
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
					                           static_cast<float>(matrix.at(6).get_double().value()),
					                           static_cast<float>(matrix.at(4).get_double().value()),
					                           static_cast<float>(matrix.at(7).get_double().value()),
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

					CommonUtilities::Vector4f vector;
					const auto& array = component["Color"].get_array().value();
					vector.x = static_cast<float>(array.at(0).get_double().value());
					vector.y = static_cast<float>(array.at(1).get_double().value());
					vector.z = static_cast<float>(array.at(2).get_double().value());
					vector.w = static_cast<float>(array.at(3).get_double().value());

					componentData.color = vector;
					componentData.intensity = static_cast<float>(component["Intensity"].get_double());

					componentData.attenuationRadius = static_cast<float>(component["AttenuationRadius"].get_double());
					componentData.outerConeAngle = static_cast<float>(component["OuterConeAngle"].get_double());
					componentData.innerConeAngle = static_cast<float>(component["InnerConeAngle"].get_double());

					componentData.falloffExponent = static_cast<float>(component["FalloffExponent"].get_double());
					actorData.components.emplace_back(componentData);
				}
				break;
				case UnrealComponentType::DirectionalLight:
				{
					CommonLightComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::DirectionalLight;
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
					                           static_cast<float>(matrix.at(6).get_double().value()),
					                           static_cast<float>(matrix.at(4).get_double().value()),
					                           static_cast<float>(matrix.at(7).get_double().value()),
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

					CommonUtilities::Vector4f vector;
					const auto& array = component["Color"].get_array().value();
					vector.x = static_cast<float>(array.at(0).get_double().value());
					vector.y = static_cast<float>(array.at(1).get_double().value());
					vector.z = static_cast<float>(array.at(2).get_double().value());
					vector.w = static_cast<float>(array.at(3).get_double().value());

					componentData.color = vector;
					componentData.intensity = static_cast<float>(component["Intensity"].get_double());
					actorData.components.emplace_back(componentData);
				}

				break;
				case UnrealComponentType::Box:
					//If we want
				{
					BoxComponentData componentData = {};

					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::Box;
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
						                       static_cast<float>(matrix.at(6).get_double().value()),
						                       static_cast<float>(matrix.at(4).get_double().value()),
						                       static_cast<float>(matrix.at(7).get_double().value()),
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

					CommonUtilities::Vector3f bounds;
					const auto& array = component["Bounds"].get_array().value();
					bounds.x = static_cast<float>(array.at(1).get_double().value());
					bounds.y = static_cast<float>(array.at(2).get_double().value());
					bounds.z = static_cast<float>(array.at(0).get_double().value());
					componentData.boundsMaxima = bounds;

					actorData.components.emplace_back(componentData);
				}
				break;
				case UnrealComponentType::Sphere:
					//If we want
				{
					BaseSphericalComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::Sphere;
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
						                       static_cast<float>(matrix.at(6).get_double().value()),
						                       static_cast<float>(matrix.at(4).get_double().value()),
						                       static_cast<float>(matrix.at(7).get_double().value()),
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

					componentData.radius = static_cast<float>(component["Radius"].get_double().value());
					actorData.components.emplace_back(componentData);
				}
				break;
				case UnrealComponentType::Capsule:
					//If we want
				{

					CapsuleComponentData componentData = {};
					ReadCommonComponent(component, componentData, UnrealComponentType::Capsule);
					componentData.radius = static_cast<float>(component["Radius"].get_double().value());
					componentData.halfHeight = static_cast<float>(component["HalfHeight"].get_double().value());
					actorData.components.emplace_back(componentData);
				}
				break;
				case UnrealComponentType::SpringArm:
					//If we want
				{
					SpringArmComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::SpringArm;
					for (const auto& tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					const auto& matrix = component["Transform"].get_array().value();
					componentData.transform = {static_cast<float>(matrix.at(5).get_double().value()),
						                       static_cast<float>(matrix.at(6).get_double().value()),
						                       static_cast<float>(matrix.at(4).get_double().value()),
						                       static_cast<float>(matrix.at(7).get_double().value()),
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
					const auto socketOffset = component["SocketOffset"].get_object().value();
					componentData.socketOffset =
					{
						static_cast<float>(socketOffset["y"].get_double().value()),
						static_cast<float>(socketOffset["z"].get_double().value()),
						static_cast<float>(socketOffset["x"].get_double().value())
					};
					componentData.armLength = static_cast<float>(component["ArmLength"].get_double().value());
					actorData.components.emplace_back(componentData);
				}
				break;
				default:
					break;
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

	BaseComponentData ConvertBaseComponent(const PerforceScene::BaseComponentData& source)
	{
		BaseComponentData component;
		component.Name = source.name;
		component.TypeID = source.typeID;
		component.Parent = source.parent;
		component.Tags = source.tags;
		component.Transform = source.transform;
		return component;
	}

	ImportedMaterialParameter ConvertMaterialParameter(const PerforceScene::MaterialParameterData& source)
	{
		ImportedMaterialParameter parameter;
		parameter.Name = source.name;
		parameter.Type = source.type;
		if (const float* scalar = std::get_if<float>(&source.value))
		{
			parameter.Value = *scalar;
		}
		else if (const CommonUtilities::Vector4f* vector = std::get_if<CommonUtilities::Vector4f>(&source.value))
		{
			parameter.Value = *vector;
		}
		else if (const PerforceScene::TextureValue* texture = std::get_if<PerforceScene::TextureValue>(&source.value))
		{
			parameter.Value = TextureValue{texture->name, texture->path};
		}
		return parameter;
	}

	std::vector<ImportedMaterial> ConvertMaterials(const std::vector<PerforceScene::MaterialData>& sourceMaterials)
	{
		std::vector<ImportedMaterial> materials;
		for (size_t materialIndex = 0; materialIndex < sourceMaterials.size(); ++materialIndex)
		{
			const PerforceScene::MaterialData* sourceMaterial = &sourceMaterials[materialIndex];
			// The Perforce parser appends completed records after filling an in-place
			// record. Collapse those adjacent duplicates at the compatibility boundary.
			while (materialIndex + 1 < sourceMaterials.size() &&
			       sourceMaterials[materialIndex + 1].name == sourceMaterial->name &&
			       sourceMaterials[materialIndex + 1].parent == sourceMaterial->parent)
			{
				sourceMaterial = &sourceMaterials[++materialIndex];
			}

			ImportedMaterial material;
			material.Name = sourceMaterial->name;
			material.Parent = sourceMaterial->parent;
			for (size_t parameterIndex = 0; parameterIndex < sourceMaterial->parameters.size(); ++parameterIndex)
			{
				const PerforceScene::MaterialParameterData* sourceParameter = &sourceMaterial->parameters[parameterIndex];
				while (parameterIndex + 1 < sourceMaterial->parameters.size() &&
				       sourceMaterial->parameters[parameterIndex + 1].name == sourceParameter->name &&
				       sourceMaterial->parameters[parameterIndex + 1].type == sourceParameter->type)
				{
					sourceParameter = &sourceMaterial->parameters[++parameterIndex];
				}
				material.Parameters.push_back(ConvertMaterialParameter(*sourceParameter));
			}
			materials.push_back(std::move(material));
		}
		return materials;
	}

	ImportedComponentData ConvertComponent(const PerforceScene::ComponentData& source)
	{
		return std::visit([](const auto& component) -> ImportedComponentData
		{
			using ComponentType = std::decay_t<decltype(component)>;
			if constexpr (std::is_same_v<ComponentType, PerforceScene::StaticMeshComponentData>)
			{
				ImportedMeshComponent mesh;
				static_cast<BaseComponentData&>(mesh) = ConvertBaseComponent(component);
				mesh.Mesh = component.mesh;
				mesh.ContentPath = component.contentPath;
				mesh.Materials = ConvertMaterials(component.materials);
				return mesh;
			}
			else if constexpr (std::is_same_v<ComponentType, PerforceScene::SpotLightComponentData>)
			{
				ImportedSpotLightComponent light;
				static_cast<BaseComponentData&>(light) = ConvertBaseComponent(component);
				light.Color = component.color;
				light.Intensity = component.intensity;
				light.FalloffExponent = component.falloffExponent;
				light.AttenuationRadius = component.attenuationRadius;
				light.InnerConeAngle = component.innerConeAngle;
				light.OuterConeAngle = component.outerConeAngle;
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, PerforceScene::PointLightComponentData>)
			{
				ImportedPointLightComponent light;
				static_cast<BaseComponentData&>(light) = ConvertBaseComponent(component);
				light.Color = component.color;
				light.Intensity = component.intensity;
				light.FalloffExponent = component.falloffExponent;
				light.AttenuationRadius = component.attenuationRadius;
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, PerforceScene::CommonLightComponentData>)
			{
				ImportedLightComponent light;
				static_cast<BaseComponentData&>(light) = ConvertBaseComponent(component);
				light.Color = component.color;
				light.Intensity = component.intensity;
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, PerforceScene::BoxComponentData>)
			{
				ImportedBoxComponent box;
				static_cast<BaseComponentData&>(box) = ConvertBaseComponent(component);
				box.BoundsMaxima = component.boundsMaxima;
				return box;
			}
			else if constexpr (std::is_same_v<ComponentType, PerforceScene::CapsuleComponentData>)
			{
				ImportedCapsuleComponent capsule;
				static_cast<BaseComponentData&>(capsule) = ConvertBaseComponent(component);
				capsule.Radius = component.radius;
				capsule.HalfHeight = component.halfHeight;
				return capsule;
			}
			else if constexpr (std::is_same_v<ComponentType, PerforceScene::BaseSphericalComponentData>)
			{
				ImportedSphereComponent sphere;
				static_cast<BaseComponentData&>(sphere) = ConvertBaseComponent(component);
				sphere.Radius = component.radius;
				return sphere;
			}
			else if constexpr (std::is_same_v<ComponentType, PerforceScene::SpringArmComponentData>)
			{
				ImportedSpringArmComponent springArm;
				static_cast<BaseComponentData&>(springArm) = ConvertBaseComponent(component);
				springArm.SocketOffset = component.socketOffset;
				springArm.ArmLength = component.armLength;
				return springArm;
			}
			else
			{
				return ConvertBaseComponent(component);
			}
		}, source);
	}

	UnrealSceneData ConvertImportedScene(const PerforceScene::UnrealSceneData& source)
	{
		UnrealSceneData scene;
		for (const PerforceScene::UnrealActorData& sourceActor : source.actors)
		{
			UnrealActorData actor;
			actor.Name = sourceActor.name;
			actor.Archetype = sourceActor.archetype;
			actor.Tags = sourceActor.tags;
			actor.Transform = sourceActor.transform;
			for (const PerforceScene::ComponentData& sourceComponent : sourceActor.components)
			{
				actor.Components.push_back(ConvertComponent(sourceComponent));
			}
			scene.Actors.push_back(std::move(actor));
		}
		return scene;
	}

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

	const BaseComponentData& GetBaseComponent(const ImportedComponentData& component)
	{
		return std::visit([](const auto& value) -> const BaseComponentData& { return value; }, component);
	}

	ComponentData ConvertCommonData(const BaseComponentData& source, const TransformData& transform)
	{
		ComponentData component;
		component.Name = source.Name;
		component.SourceParent = source.Parent;
		component.Tags = source.Tags;
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

	RuntimeConversionResult ConvertRuntimeScene(const UnrealSceneData& source)
	{
		RuntimeConversionResult result;
		SceneData scene;
		for (const UnrealActorData& sourceActor : source.Actors)
		{
			const std::optional<TransformData> actorTransform = ConvertTransform(sourceActor.Transform);
			if (!actorTransform)
			{
				result.Diagnostics.push_back({sourceActor.Name, "actor transform cannot be decomposed"});
				continue;
			}

			ActorRecord actor;
			actor.Name = sourceActor.Name;
			actor.Archetype = sourceActor.Archetype;
			actor.Tags = sourceActor.Tags;
			actor.Transform = *actorTransform;

			for (const ImportedComponentData& sourceComponent : sourceActor.Components)
			{
				const BaseComponentData& base = GetBaseComponent(sourceComponent);
				const std::string context = sourceActor.Name + "/" + base.Name;
				const std::optional<TransformData> componentTransform = ConvertTransform(base.Transform);
				if (!componentTransform)
				{
					result.Diagnostics.push_back({context, "component transform cannot be decomposed"});
					continue;
				}

				std::visit([&](const auto& imported)
				{
					using ImportedType = std::decay_t<decltype(imported)>;
					const ComponentData common = ConvertCommonData(base, *componentTransform);
					if constexpr (std::is_same_v<ImportedType, ImportedMeshComponent>)
					{
						StaticMeshData mesh;
						mesh.Common = common;
						mesh.MeshName = imported.Mesh;
						mesh.ContentPath = imported.ContentPath;
						mesh.Mesh = AssetId{imported.ContentPath};
						for (const ImportedMaterial& importedMaterial : imported.Materials)
						{
							MaterialInstanceData material;
							material.Name = importedMaterial.Name;
							material.Parent = AssetId{importedMaterial.Parent.empty() ? importedMaterial.Name : importedMaterial.Parent};
							for (const ImportedMaterialParameter& importedParameter : importedMaterial.Parameters)
							{
								MaterialParameterData parameter;
								parameter.Name = importedParameter.Name;
								if (const float* scalar = std::get_if<float>(&importedParameter.Value))
								{
									parameter.Value = *scalar;
								}
								else if (const CommonUtilities::Vector4f* vector =
								             std::get_if<CommonUtilities::Vector4f>(&importedParameter.Value))
								{
									parameter.Value = *vector;
								}
								else if (const TextureValue* texture = std::get_if<TextureValue>(&importedParameter.Value))
								{
									parameter.Value = AssetId{texture->Path};
								}
								material.Parameters.push_back(std::move(parameter));
							}
							mesh.Materials.push_back(std::move(material));
						}
						if (base.TypeID == UnrealComponentType::SkeletalMesh)
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
					else if constexpr (std::is_same_v<ImportedType, ImportedSpotLightComponent>)
					{
						SpotLightData light;
						light.Common = common;
						light.Color = {imported.Color.x, imported.Color.y, imported.Color.z};
						light.ColorAlpha = imported.Color.w;
						light.Intensity = imported.Intensity;
						light.Radius = imported.AttenuationRadius * UnrealUnitScale;
						light.FalloffExponent = imported.FalloffExponent;
						light.InnerConeDegrees = imported.InnerConeAngle;
						light.OuterConeDegrees = imported.OuterConeAngle;
						actor.Components.push_back(std::move(light));
					}
					else if constexpr (std::is_same_v<ImportedType, ImportedPointLightComponent>)
					{
						PointLightData light;
						light.Common = common;
						light.Color = {imported.Color.x, imported.Color.y, imported.Color.z};
						light.ColorAlpha = imported.Color.w;
						light.Intensity = imported.Intensity;
						light.Radius = imported.AttenuationRadius * UnrealUnitScale;
						light.FalloffExponent = imported.FalloffExponent;
						actor.Components.push_back(std::move(light));
					}
					else if constexpr (std::is_same_v<ImportedType, ImportedLightComponent>)
					{
						DirectionalLightData light;
						light.Common = common;
						light.Color = {imported.Color.x, imported.Color.y, imported.Color.z};
						light.ColorAlpha = imported.Color.w;
						light.Intensity = imported.Intensity;
						actor.Components.push_back(std::move(light));
					}
					else if (base.TypeID == UnrealComponentType::SceneComponent &&
					         std::find(base.Tags.begin(), base.Tags.end(), "ActiveCamera") != base.Tags.end())
					{
						actor.Components.push_back(CameraData{common});
					}
					else if (base.TypeID == UnrealComponentType::SceneComponent)
					{
						actor.Components.push_back(SceneComponentData{common});
					}
					else
					{
						PlaceholderComponentData placeholder;
						placeholder.Common = common;
						placeholder.Type = ConvertPlaceholderType(base.TypeID);
						if constexpr (std::is_same_v<ImportedType, ImportedBoxComponent>)
						{
							placeholder.Properties = BoxPlaceholderData{imported.BoundsMaxima * UnrealUnitScale};
						}
						else if constexpr (std::is_same_v<ImportedType, ImportedSphereComponent>)
						{
							placeholder.Properties = SpherePlaceholderData{imported.Radius * UnrealUnitScale};
						}
						else if constexpr (std::is_same_v<ImportedType, ImportedCapsuleComponent>)
						{
							placeholder.Properties = CapsulePlaceholderData{
								imported.Radius * UnrealUnitScale, imported.HalfHeight * UnrealUnitScale};
						}
						else if constexpr (std::is_same_v<ImportedType, ImportedSpringArmComponent>)
						{
							placeholder.Properties = SpringArmPlaceholderData{
								imported.SocketOffset * UnrealUnitScale, imported.ArmLength * UnrealUnitScale};
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
		const PerforceScene::UnrealSceneData importedScene = PerforceScene::ImportScene(jsonPath);
		if (!importedScene.parsed)
		{
			result.Diagnostics.push_back({jsonPath.string(), "could not parse scene JSON"});
			return result;
		}
		const UnrealSceneData sourceScene = ConvertImportedScene(importedScene);
		RuntimeConversionResult converted = ConvertRuntimeScene(sourceScene);
		result.Diagnostics = std::move(converted.Diagnostics);
		result.Data = std::move(converted.Scene);
	}
	catch (const std::exception& error)
	{
		result.Diagnostics.push_back({jsonPath.string(), error.what()});
	}
	return result;
}
