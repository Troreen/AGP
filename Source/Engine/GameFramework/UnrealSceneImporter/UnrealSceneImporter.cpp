#include "UnrealSceneImporter.h"
#include <SimdJson/simdjson.h>

UnrealSceneData UnrealSceneImporter::ImportScene(std::filesystem::path aJSONPath)
{
	simdjson::padded_string json = simdjson::padded_string::load(aJSONPath.c_str());
	simdjson::dom::parser parser;
	simdjson::dom::object root;
	auto error = parser.parse(json).get(root);

	// TODO: create structs with level data
	//std::cout << root["Actors"].get_array().size() << std::endl;

	if (error)
	{
		return {};
	}

	UnrealSceneData unrealData = {};

	for (auto actor : root["Actors"])
	{
		UnrealActorData actorData = {};
		//std::cout << actor["Name"].get_c_str() << std::endl;
		actorData.name = actor["Name"];
		actorData.archetype = actor["Archetype"];
		for (auto tag : actor["Tags"])
		{
			std::string actorTag(tag.get_c_str());
			actorData.tags.emplace_back(actorTag);
		}

		{
			auto matrix = actor["Transform"].get_array().value();
			actorData.transform =
			{
				static_cast<float>(matrix.at(5).get_double().value()),
				static_cast<float>(matrix.at(6).get_double().value()),
				static_cast<float>(matrix.at(4).get_double().value()),
				static_cast<float>(matrix.at(3).get_double().value()),
				static_cast<float>(matrix.at(9).get_double().value()),
				static_cast<float>(matrix.at(10).get_double().value()),
				static_cast<float>(matrix.at(8).get_double().value()),
				static_cast<float>(matrix.at(7).get_double().value()),
				static_cast<float>(matrix.at(1).get_double().value()),
				static_cast<float>(matrix.at(3).get_double().value()),
				static_cast<float>(matrix.at(0).get_double().value()),
				static_cast<float>(matrix.at(11).get_double().value()),
				static_cast<float>(matrix.at(12).get_double().value()),
				static_cast<float>(matrix.at(13).get_double().value()),
				static_cast<float>(matrix.at(14).get_double().value()),
				static_cast<float>(matrix.at(15).get_double().value())

			};
		}

		for (auto component : actor["Components"])
		{
			switch (static_cast<UnrealComponentType>(component["TypeID"].get_int64().value()))
			{
				case UnrealComponentType::Custom:
				{
					BaseComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::Custom;
					for (auto tag : component["Tags"])
					{
						std::string componentTag(tag.get_c_str());
						componentData.tags.emplace_back(componentTag);
					}

					auto matrix = component["Transform"].get_array().value();
					componentData.transform =
					{
						static_cast<float>(matrix.at(5).get_double().value()),
						static_cast<float>(matrix.at(6).get_double().value()),
						static_cast<float>(matrix.at(4).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(9).get_double().value()),
						static_cast<float>(matrix.at(10).get_double().value()),
						static_cast<float>(matrix.at(8).get_double().value()),
						static_cast<float>(matrix.at(7).get_double().value()),
						static_cast<float>(matrix.at(1).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(0).get_double().value()),
						static_cast<float>(matrix.at(11).get_double().value()),
						static_cast<float>(matrix.at(12).get_double().value()),
						static_cast<float>(matrix.at(13).get_double().value()),
						static_cast<float>(matrix.at(14).get_double().value()),
						static_cast<float>(matrix.at(15).get_double().value())

					};
				}
					break;
				case UnrealComponentType::SceneComponent:
					//If we want
					break;
				case UnrealComponentType::StaticMesh:
				{
					StaticMeshComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::StaticMesh;
					for (auto tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					auto matrix = component["Transform"].get_array().value();
					componentData.transform =
					{
						static_cast<float>(matrix.at(5).get_double().value()),
						static_cast<float>(matrix.at(6).get_double().value()),
						static_cast<float>(matrix.at(4).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(9).get_double().value()),
						static_cast<float>(matrix.at(10).get_double().value()),
						static_cast<float>(matrix.at(8).get_double().value()),
						static_cast<float>(matrix.at(7).get_double().value()),
						static_cast<float>(matrix.at(1).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(0).get_double().value()),
						static_cast<float>(matrix.at(11).get_double().value()),
						static_cast<float>(matrix.at(12).get_double().value()),
						static_cast<float>(matrix.at(13).get_double().value()),
						static_cast<float>(matrix.at(14).get_double().value()),
						static_cast<float>(matrix.at(15).get_double().value())

					};

					componentData.mesh = component["Mesh"];

					for (auto material : component["Materials"])
					{

						MaterialData& materialData = componentData.materials.emplace_back();
						materialData.name = material["Name"];
						materialData.parent = material["Parent"].has_value() ?  material["Parent"].get_c_str().value() : "";

						for (auto parameter : material["Parameters"].get_array().value())
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
								}
									break;
								case MaterialType::Vector3f:
								{
									CommonUtilities::Vector4f vector;

									auto value = parameter["Value"].get_array().value();

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
									auto value = parameter["Value"].get_object().value();
									textureValue.name = value["Name"].get_string().value();
									textureValue.path = value["Path"].get_string().value();

									parameterData.value = textureValue;
								}
									break;
								default:
									break;
							}
						}
					}
				}
					break;
				case UnrealComponentType::SkeletalMesh:
					//If we want
					break;
				case UnrealComponentType::PointLight:
				{
					PointLightComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];


					componentData.typeID = UnrealComponentType::SpotLight;
					for (auto tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					auto matrix = component["Transform"].get_array().value();
					componentData.transform =
					{
						static_cast<float>(matrix.at(5).get_double().value()),
						static_cast<float>(matrix.at(6).get_double().value()),
						static_cast<float>(matrix.at(4).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(9).get_double().value()),
						static_cast<float>(matrix.at(10).get_double().value()),
						static_cast<float>(matrix.at(8).get_double().value()),
						static_cast<float>(matrix.at(7).get_double().value()),
						static_cast<float>(matrix.at(1).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(0).get_double().value()),
						static_cast<float>(matrix.at(11).get_double().value()),
						static_cast<float>(matrix.at(12).get_double().value()),
						static_cast<float>(matrix.at(13).get_double().value()),
						static_cast<float>(matrix.at(14).get_double().value()),
						static_cast<float>(matrix.at(15).get_double().value())

					};

					CommonUtilities::Vector4f vector;
					auto array = component["Color"].get_array().value();
					vector.x = static_cast<float>(array.at(0).get_double().value());
					vector.y = static_cast<float>(array.at(0).get_double().value());
					vector.z = static_cast<float>(array.at(0).get_double().value());
					vector.w = static_cast<float>(array.at(0).get_double().value());

					componentData.color = vector;
					componentData.intensity = static_cast<float>(component["Intensity"].get_double());

					componentData.attenuationRadius = static_cast<float>(component["AttenuationRadius"].get_double());
					componentData.falloffExponent = static_cast<float>(component["FalloffExponent"].get_double());
				}

					break;
				case UnrealComponentType::SpotLight:
				{
					SpotLightComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];


					componentData.typeID = UnrealComponentType::SpotLight;
					for (auto tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					auto matrix = component["Transform"].get_array().value();
					componentData.transform = 
					{
						static_cast<float>(matrix.at(5).get_double().value()),
						static_cast<float>(matrix.at(6).get_double().value()),
						static_cast<float>(matrix.at(4).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(9).get_double().value()),
						static_cast<float>(matrix.at(10).get_double().value()),
						static_cast<float>(matrix.at(8).get_double().value()),
						static_cast<float>(matrix.at(7).get_double().value()),
						static_cast<float>(matrix.at(1).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(0).get_double().value()),
						static_cast<float>(matrix.at(11).get_double().value()),
						static_cast<float>(matrix.at(12).get_double().value()),
						static_cast<float>(matrix.at(13).get_double().value()),
						static_cast<float>(matrix.at(14).get_double().value()),
						static_cast<float>(matrix.at(15).get_double().value())

					};

					CommonUtilities::Vector4f vector;
					auto array = component["Color"].get_array().value();
					vector.x = static_cast<float>(array.at(0).get_double().value());
					vector.y = static_cast<float>(array.at(0).get_double().value());
					vector.z = static_cast<float>(array.at(0).get_double().value());
					vector.w = static_cast<float>(array.at(0).get_double().value());

					componentData.color = vector;
					componentData.intensity = static_cast<float>(component["Intensity"].get_double());

					componentData.attenuationRadius = static_cast<float>(component["AttenuationRadius"].get_double());
					componentData.outerConeAngle = static_cast<float>(component["OuterConeAngle"].get_double());
					componentData.innerConeAngle = static_cast<float>(component["InnerConeAngle"].get_double());

					componentData.falloffExponent = static_cast<float>(component["FalloffExponent"].get_double());
				}
					break;
				case UnrealComponentType::DirectionalLight:
				{
					CommonLightComponentData componentData = {};
					componentData.name = component["Name"];
					componentData.parent = component["Parent"];
					componentData.typeID = UnrealComponentType::DirectionalLight;
					for (auto tag : component["Tags"])
					{
						std::string componentTag = tag.get_c_str().value();
						componentData.tags.emplace_back(componentTag);
					}

					auto matrix = component["Transform"].get_array().value();
					componentData.transform =
					{
						static_cast<float>(matrix.at(5).get_double().value()),
						static_cast<float>(matrix.at(6).get_double().value()),
						static_cast<float>(matrix.at(4).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(9).get_double().value()),
						static_cast<float>(matrix.at(10).get_double().value()),
						static_cast<float>(matrix.at(8).get_double().value()),
						static_cast<float>(matrix.at(7).get_double().value()),
						static_cast<float>(matrix.at(1).get_double().value()),
						static_cast<float>(matrix.at(3).get_double().value()),
						static_cast<float>(matrix.at(0).get_double().value()),
						static_cast<float>(matrix.at(11).get_double().value()),
						static_cast<float>(matrix.at(12).get_double().value()),
						static_cast<float>(matrix.at(13).get_double().value()),
						static_cast<float>(matrix.at(14).get_double().value()),
						static_cast<float>(matrix.at(15).get_double().value())

					};

					CommonUtilities::Vector4f vector;
					auto array = component["Color"].get_array().value();
					vector.x = static_cast<float>(array.at(0).get_double().value());
					vector.y = static_cast<float>(array.at(0).get_double().value());
					vector.z = static_cast<float>(array.at(0).get_double().value());
					vector.w = static_cast<float>(array.at(0).get_double().value());

					componentData.color = vector;
					componentData.intensity = static_cast<float>(component["Intensity"].get_double());
				}

					break;
				case UnrealComponentType::Box:
					//If we want

					break;
				case UnrealComponentType::Sphere:
					//If we want

					break;
				case UnrealComponentType::Capsule:
					//If we want

					break;
				case UnrealComponentType::SpringArm:
					//If we want

					break;
				default:
					break;
			}
		}
	}

	return unrealData;
}
