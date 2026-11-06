#include "GameFramework.pch.h"
#include "UnrealSceneImporter.h"
#include "UnrealSceneStructs.h"
#include "Maths.hpp"
#include "Quaternion.hpp"
#include <SimdJson/simdjson.h>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace UnrealScene
{
	class MaterialImportError : public std::invalid_argument
	{
	public:
		MaterialImportError(std::string aContext, const std::string& aMessage)
			: std::invalid_argument(aMessage), Context(std::move(aContext))
		{
		}

		std::string Context;
	};

	CommonUtilities::Matrix4f ReadSceneTransform(simdjson::dom::element anObject, const std::string& aContext)
	{
		const auto transform = anObject["Transform"];
		if (transform.error() == simdjson::NO_SUCH_FIELD)
		{
			return {};
		}
		const auto matrix = transform.get_array().value();
		if (matrix.size() != 16)
		{
			throw MaterialImportError(aContext, "Transform must contain 16 numbers");
		}
		CommonUtilities::Matrix4f result;
		constexpr size_t permutation[] = { 1, 2, 0, 3 };
		for (size_t row = 0; row < 4; ++row)
		{
			for (size_t column = 0; column < 4; ++column)
			{
				const size_t sourceIndex = column == 3 ? row * 4 + column : permutation[row] * 4 + permutation[column];
				result(static_cast<int>(row + 1), static_cast<int>(column + 1)) =
					static_cast<float>(matrix.at(sourceIndex).get_double().value());
			}
		}
		return result;
	}

	CommonUtilities::Vector3f ReadCustomVector(simdjson::dom::element aValue)
	{
		CommonUtilities::Vector3f vector;
		vector.x = static_cast<float>(aValue["x"].get_double().value());
		vector.y = static_cast<float>(aValue["y"].get_double().value());
		vector.z = static_cast<float>(aValue["z"].get_double().value());
		if (!std::isfinite(vector.x) || !std::isfinite(vector.y) || !std::isfinite(vector.z))
		{
			throw std::invalid_argument("Custom vector axes must be finite numbers");
		}
		return vector;
	}

	CustomTransformData ReadCustomTransform(simdjson::dom::object aTransformObject)
	{
		CustomTransformData transform;
		transform.Translation = ReadCustomVector(aTransformObject["translation"].value());
		transform.Scale = ReadCustomVector(aTransformObject["scale3D"].value());
		const auto rotation = aTransformObject["rotation"].value();
		transform.Rotation = {
			static_cast<float>(rotation["x"].get_double().value()),
			static_cast<float>(rotation["y"].get_double().value()),
			static_cast<float>(rotation["z"].get_double().value()),
			static_cast<float>(rotation["w"].get_double().value())
		};
		if (!std::isfinite(transform.Rotation.x) || !std::isfinite(transform.Rotation.y) ||
		    !std::isfinite(transform.Rotation.z) || !std::isfinite(transform.Rotation.w))
		{
			throw std::invalid_argument("Transform rotation quaternion values must be finite numbers");
		}
		return transform;
	}

	CustomComponentVariables ReadCustomVariables(simdjson::dom::object someVariables)
	{
		CustomComponentVariables customVariables;
		for (const auto& variable : someVariables)
		{
			const std::string variableName(variable.key);
			if (customVariables.Contains(variableName))
			{
				throw std::invalid_argument("Duplicate custom variable: " + variableName);
			}
			switch (variable.value.type())
			{
			case simdjson::dom::element_type::INT64:
				customVariables.IntVariables.emplace(variableName, variable.value.get_int64().value());
				break;
			case simdjson::dom::element_type::BOOL:
				customVariables.BoolVariables.emplace(variableName, variable.value.get_bool().value());
				break;
			case simdjson::dom::element_type::STRING:
				customVariables.StringVariables.emplace(variableName, variable.value.get_string().value());
				break;
			case simdjson::dom::element_type::DOUBLE:
			{
				const float value = static_cast<float>(variable.value.get_double().value());
				if (!std::isfinite(value))
				{
					throw std::invalid_argument(variableName + " must be a finite float");
				}
				customVariables.FloatVariables.emplace(variableName, value);
				break;
			}
			case simdjson::dom::element_type::OBJECT:
				try
				{
					const auto object = variable.value.get_object().value();
					if (object.size() != 3)
					{
						throw std::invalid_argument("Expected a vector or transform object");
					}
					if (object["rotation"].error() == simdjson::SUCCESS)
					{
						customVariables.TransformVariables.emplace(variableName, ReadCustomTransform(object));
					}
					else
					{
						customVariables.VectorVariables.emplace(variableName, ReadCustomVector(variable.value));
					}
					break;
				}
				catch (const std::exception& error)
				{
					throw std::invalid_argument(variableName + ": " + error.what());
				}
			default:
				throw std::invalid_argument(variableName + " has an unsupported custom variable type");
			}
		}
		return customVariables;
	}

	void ReadCommonComponent(simdjson::dom::element component, BaseComponentData& outComponent, UnrealComponentType type)
	{
		outComponent.name = component["Name"];
		const auto& parentElement = component.at_key("Parent");
		if (parentElement.error() != simdjson::NO_SUCH_FIELD)
		{
			outComponent.parent = parentElement;
		}
		outComponent.typeID = type;
		if (type == UnrealComponentType::Custom)
		{
			std::string_view customType;
			const auto typeError = component["CustomType"].get(customType);
			if (typeError != simdjson::NO_SUCH_FIELD)
			{
				if (typeError != simdjson::SUCCESS || customType.empty())
				{
					throw std::invalid_argument(outComponent.name + ": CustomType must be a nonempty string");
				}
				outComponent.customType = customType;
				simdjson::dom::object variables;
				if (component["Custom"].get(variables) != simdjson::SUCCESS)
				{
					throw std::invalid_argument(outComponent.name + ": Custom must be an object");
				}
				outComponent.customVariables = ReadCustomVariables(variables);
			}
		}
		for (const auto& tag : component["Tags"])
		{
			outComponent.tags.emplace_back(tag.get_c_str().value());
		}

		outComponent.transform = ReadSceneTransform(component, "Component '" + outComponent.name + "'");
	}

	CommonUtilities::Matrix4x4<float> UnrealToCommonUtilitiesMatrix(const std::vector<float>& aJsonTransform)
	{
		CommonUtilities::Matrix4x4<float> unrealMatrix;

		int jsonIndex = 0;

		for (const auto& element : aJsonTransform)
		{
			int row = jsonIndex / 4 + 1;
			int col = jsonIndex % 4 + 1;

			unrealMatrix(row, col) = element;

			jsonIndex++;
		}

		CommonUtilities::Matrix4x4<float> matrixRotation;

		matrixRotation[0][0] = unrealMatrix[1][0];
		matrixRotation[0][1] = unrealMatrix[1][1];
		matrixRotation[0][2] = unrealMatrix[1][2];

		matrixRotation[1][0] = unrealMatrix[2][0];
		matrixRotation[1][1] = unrealMatrix[2][1];
		matrixRotation[1][2] = unrealMatrix[2][2];

		matrixRotation[2][0] = unrealMatrix[0][0];
		matrixRotation[2][1] = unrealMatrix[0][1];
		matrixRotation[2][2] = unrealMatrix[0][2];


		CommonUtilities::Matrix4x4<float> returnMatrix;

		returnMatrix[0][0] = matrixRotation[0][1];
		returnMatrix[1][0] = matrixRotation[1][1];
		returnMatrix[2][0] = matrixRotation[2][1];

		returnMatrix[0][1] = matrixRotation[0][2];
		returnMatrix[1][1] = matrixRotation[1][2];
		returnMatrix[2][1] = matrixRotation[2][2];

		returnMatrix[0][2] = matrixRotation[0][0];
		returnMatrix[1][2] = matrixRotation[1][0];
		returnMatrix[2][2] = matrixRotation[2][0];

		returnMatrix[3][0] = unrealMatrix[3][1];
		returnMatrix[3][1] = unrealMatrix[3][2];
		returnMatrix[3][2] = unrealMatrix[3][0];

		return returnMatrix;
	}

	CommonUtilities::Matrix4f UnrealToDirectX(const CommonUtilities::Matrix4f& aUEMatrix)
	{
		CommonUtilities::Matrix4f result;
		constexpr int p[4] = { 2, 3, 1, 4 };

		for (int r = 0; r < 4; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				result(r + 1, c + 1) = aUEMatrix(p[r], p[c]);
			}
		}

		return result;
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

	void ReadMaterialParameters(const simdjson::dom::element& aMaterialJson, std::vector<MaterialParameterData>& aParameterData)
	{
		for (const auto& parameter : aMaterialJson["Parameters"].get_array().value())
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
					textureValue.path = std::filesystem::path(value["AssetImportPath"].get_string().value()).stem().string();

					parameterData.value = textureValue;
				}
				break;

				default:
					continue;
			}
			aParameterData.emplace_back(parameterData);
		}
	}

	struct MaterialTemplate
	{
		std::string Parent;
		std::vector<MaterialParameterData> Parameters;
	};

	struct ResolvedMaterialTemplate
	{
		std::string RootName;
		std::vector<MaterialParameterData> Parameters;
	};

	class MaterialTemplates
	{
	public:
		explicit MaterialTemplates(simdjson::dom::element someMaterials)
		{
			for (const auto& material : someMaterials.get_array().value())
			{
				const std::string name(material["Name"].get_string().value());
				MaterialTemplate definition;
				const auto parent = material["Parent"];
				if (parent.error() != simdjson::NO_SUCH_FIELD)
				{
					definition.Parent = parent.get_string().value();
				}
				if (definition.Parent == "Self")
				{
					definition.Parent.clear();
				}
				ReadMaterialParameters(material, definition.Parameters);
				if (name.empty() || !myDefinitions.emplace(name, definition).second)
				{
					throw MaterialImportError("Material '" + name + "'", "Duplicate or empty material name");
				}
				if (!definition.Parent.empty())
				{
					continue;
				}

				MasterMaterialData master;
				master.name = name;
				master.domain = material["Domain"].operator size_t();
				master.shadingModel = material["ShadingModel"].operator size_t();
				master.blendMode = material["BlendMode"].operator size_t();
				const auto baseColor = material["BaseColor"].get_array().value();
				master.baseColor = {
					static_cast<float>(baseColor.at(0).get_double().value()),
					static_cast<float>(baseColor.at(1).get_double().value()),
					static_cast<float>(baseColor.at(2).get_double().value()),
					static_cast<float>(baseColor.at(3).get_double().value())
				};
				master.metallic = static_cast<float>(material["Metallic"].get_double().value());
				master.roughness = static_cast<float>(material["Roughness"].get_double().value());
				const auto twoSided = material["TwoSided"];
				if (twoSided.error() != simdjson::NO_SUCH_FIELD)
				{
					master.twoSided = twoSided.get_bool().value();
				}
				master.parameters = definition.Parameters;
				Masters.push_back(std::move(master));
			}

			// Resolve after collection so child-before-parent export order is valid.
			for (const auto& [name, definition] : myDefinitions)
			{
				Resolve(name);
			}
		}

		MaterialData ReadInstance(simdjson::dom::element aMaterial, const std::string& aContext) const
		{
			MaterialData result;
			result.name = aMaterial["Name"].get_string().value();
			const auto parent = aMaterial["Parent"];
			if (parent.error() != simdjson::NO_SUCH_FIELD)
			{
				result.parent = parent.get_string().value();
			}

			// An explicit component parent takes precedence over any same-named
			// template. Empty/Self references may name a top-level template directly.
			const std::string templateName = result.parent.empty() || result.parent == "Self" ? result.name : result.parent;
			const auto resolved = myResolved.find(templateName);
			if (resolved != myResolved.end())
			{
				result.parent = resolved->second.RootName;
				result.parameters = resolved->second.Parameters;
			}
			else if (!result.parent.empty() && result.parent != "Self")
			{
				throw MaterialImportError(aContext + "/Material '" + result.name + "'", "missing parent '" + result.parent + "'");
			}
			ReadMaterialParameters(aMaterial, result.parameters);
			return result;
		}

		std::vector<MasterMaterialData> Masters;

	private:
		const ResolvedMaterialTemplate& Resolve(const std::string& aName)
		{
			const auto cached = myResolved.find(aName);
			if (cached != myResolved.end())
			{
				return cached->second;
			}
			if (!myResolving.insert(aName).second)
			{
				throw MaterialImportError("Material '" + aName + "'", "Material inheritance cycle");
			}
			const MaterialTemplate& definition = myDefinitions.at(aName);
			ResolvedMaterialTemplate result;
			if (definition.Parent.empty())
			{
				result.RootName = aName;
			}
			else
			{
				if (!myDefinitions.contains(definition.Parent))
				{
					throw MaterialImportError("Material '" + aName + "'", "missing parent '" + definition.Parent + "'");
				}
				result = Resolve(definition.Parent);
				result.Parameters.insert(result.Parameters.end(), definition.Parameters.begin(), definition.Parameters.end());
			}
			myResolving.erase(aName);
			return myResolved.emplace(aName, std::move(result)).first->second;
		}

		std::unordered_map<std::string, MaterialTemplate> myDefinitions;
		std::unordered_map<std::string, ResolvedMaterialTemplate> myResolved;
		std::unordered_set<std::string> myResolving;
	};

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
		
		const MaterialTemplates materialTemplates(root["MasterMaterials"].value());
		unrealData.materials = materialTemplates.Masters;

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

			actorData.transform = ReadSceneTransform(actor, "Actor '" + actorData.name + "'");

			for (const auto& component : actor["Components"])
			{
				const UnrealComponentType type = static_cast<UnrealComponentType>(component["TypeID"].get_int64().value());
				switch (type)
				{
					case UnrealComponentType::Landscape:
						// Native Landscape records describe authoring paint. The linked
						// static mesh carries runtime geometry and exported material bindings.
						break;
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
							if (material.at_key("Name").error() == simdjson::NO_SUCH_FIELD)
							{
								continue;
							}

							componentData.materials.push_back(materialTemplates.ReadInstance(material, actorData.name + "/" + componentData.name));
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

						try
						{
							const auto bounds = ReadCustomVector(component["Bounds"].value());
							componentData.boundsMaxima = {bounds.y, bounds.z, bounds.x};
						}
						catch (const std::exception& error)
						{
							throw std::invalid_argument(componentData.name + ": invalid Bounds: " + error.what());
						}

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
					case UnrealComponentType::SkyLight:
					{
						SkyLightComponentData componentData = {};
						ReadCommonComponent(component, componentData, UnrealComponentType::SkyLight);

						componentData.EnvironmentCubeTexture = component["CubeMap"]["Path"].get_string().value();

						const auto color = component["Color"].get_array().value();
						componentData.AmbientLightColor = {
							static_cast<float>(color.at(0).get_double().value()),
							static_cast<float>(color.at(1).get_double().value()),
							static_cast<float>(color.at(2).get_double().value()),
						};
						componentData.AmbientLightIntensity = static_cast<float>(component["Intensity"].get_double().value());
						actorData.components.push_back(std::move(componentData));
					}
					break;
					case UnrealComponentType::Camera:
					{
						CameraComponentData componentData = {};
						ReadCommonComponent(component, componentData, UnrealComponentType::Camera);
						componentData.fieldOfView = static_cast<float>(component["FOV"].get_double().value());
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

	const UnrealScene::BaseComponentData& GetBaseComponent(const UnrealScene::ComponentData& component)
	{
		return std::visit([](const auto& value) -> const UnrealScene::BaseComponentData& { return value; }, component);
	}

	ComponentData ConvertCommonData(const UnrealScene::BaseComponentData& source)
	{
		ComponentData component;
		component.Name = source.name;
		component.SourceParent = source.parent;
		component.Tags = source.tags;
		component.Transform = source.transform;
		return component;
	}

	PlaceholderComponentType ConvertPlaceholderType(UnrealComponentType type)
	{
		switch (type)
		{
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

		for (const UnrealScene::MasterMaterialData& sourceMaterial : source.materials)
		{
			MasterMaterialRecord material;
			material.Name = sourceMaterial.name;
			material.Domain = sourceMaterial.domain;
			material.ShadingModel = sourceMaterial.shadingModel;
			material.BlendMode = sourceMaterial.blendMode;
			material.BaseColor = sourceMaterial.baseColor;
			material.Metallic = sourceMaterial.metallic;
			material.Roughness = sourceMaterial.roughness;
			material.TwoSided = sourceMaterial.twoSided;

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

			scene.Materials.emplace_back(std::move(material));
		}

		for (const UnrealScene::UnrealActorData& sourceActor : source.actors)
		{
			ActorRecord actor;
			actor.Name = sourceActor.name;
			actor.Archetype = sourceActor.archetype;
			actor.Tags = sourceActor.tags;
			actor.Transform = sourceActor.transform;

			const auto isCollider = [](const UnrealScene::ComponentData& component)
			{
				const auto type = GetBaseComponent(component).typeID;
				return type == UnrealComponentType::Box || type == UnrealComponentType::Sphere;
			};

			for (const UnrealScene::ComponentData& sourceComponent : sourceActor.components)
			{
				const UnrealScene::BaseComponentData& baseComponentData = GetBaseComponent(sourceComponent);
				const std::string context = sourceActor.name + "/" + baseComponentData.name;
				// Colliders may attach directly to the root. Deeper chains need transforms this importer does not rebuild.
				if (isCollider(sourceComponent) && !baseComponentData.parent.empty())
				{
					const auto parent = std::find_if(sourceActor.components.begin(), sourceActor.components.end(), [&](const auto& component)
					{
						return GetBaseComponent(component).name == baseComponentData.parent;
					});
					if (parent == sourceActor.components.end() || !GetBaseComponent(*parent).parent.empty())
					{
						result.Diagnostics.push_back({context, "Collider Parent must be the Actor's root component; nested attachments are unsupported"});
						continue;
					}
				}

				std::visit([&](const auto& imported)
					{
						using SourceType = std::decay_t<decltype(imported)>;
						const ComponentData common = ConvertCommonData(baseComponentData);
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
								material.Parent = sourceMaterial.parent;
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
							if (baseComponentData.typeID == UnrealComponentType::SkeletalMesh)
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
							light.Color = { imported.color.x, imported.color.y, imported.color.z };
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
							light.Color = { imported.color.x, imported.color.y, imported.color.z };
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
							light.Color = { imported.color.x, imported.color.y, imported.color.z };
							light.ColorAlpha = imported.color.w;
							light.Intensity = imported.intensity;
							actor.Components.push_back(std::move(light));
						}
						else if constexpr (std::is_same_v<SourceType, UnrealScene::SkyLightComponentData>)
						{
							scene.EnvironmentCubeTexture = std::filesystem::path(imported.EnvironmentCubeTexture).stem().string();
							scene.AmbientLightColor = imported.AmbientLightColor;
							scene.AmbientLightIntensity = imported.AmbientLightIntensity;
						}
						else if constexpr (std::is_same_v<SourceType, UnrealScene::CameraComponentData>)
						{
							actor.Components.push_back(CameraData{ common, imported.fieldOfView });
						}
						else if constexpr (std::is_same_v<SourceType, UnrealScene::BoxComponentData>)
						{
							// Exported bounds are distances from the center to each face. Leave transform scale for runtime.
							ColliderData collider;
							collider.Common = common;
							collider.Shape = BoxColliderShape{imported.boundsMaxima * UnrealUnitScale};
							actor.Components.push_back(std::move(collider));
						}
						else if constexpr (std::is_same_v<SourceType, UnrealScene::BaseSphericalComponentData>)
						{
							// Keep the radius local, just like box dimensions, so scale is applied once by the collider transform.
							ColliderData collider;
							collider.Common = common;
							collider.Shape = SphereColliderShape{imported.radius * UnrealUnitScale};
							actor.Components.push_back(std::move(collider));
						}
						else if (baseComponentData.typeID == UnrealComponentType::SceneComponent)
						{
							actor.Components.push_back(SceneComponentData{ common });
						}
						else if (baseComponentData.typeID == UnrealComponentType::Custom && !baseComponentData.customType.empty())
						{
							CustomComponentData customComponentData;
							customComponentData.Common = common;
							customComponentData.CustomType = baseComponentData.customType;
							customComponentData.Variables = baseComponentData.customVariables;
							actor.Components.push_back(std::move(customComponentData));
						}
						else
						{
							PlaceholderComponentData placeholder;
							placeholder.Common = common;
							placeholder.Type = ConvertPlaceholderType(baseComponentData.typeID);
							if constexpr (std::is_same_v<SourceType, UnrealScene::CapsuleComponentData>)
							{
								placeholder.Properties = CapsulePlaceholderData{
									imported.radius * UnrealUnitScale, imported.halfHeight * UnrealUnitScale };
							}
							else if constexpr (std::is_same_v<SourceType, UnrealScene::SpringArmComponentData>)
							{
								placeholder.Properties = SpringArmPlaceholderData{
									imported.socketOffset * UnrealUnitScale, imported.armLength * UnrealUnitScale };
							}
							actor.Components.push_back(std::move(placeholder));
						}
					}, sourceComponent);
			}
			scene.Actors.emplace_back(std::move(actor));
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
	UnrealScene::UnrealSceneData importedScene;
	try
	{
		importedScene = UnrealScene::ImportScene(jsonPath);
	}
	catch (const UnrealScene::MaterialImportError& error)
	{
		result.Diagnostics.push_back({ error.Context, error.what() });
		return result;
	}
	catch (const std::exception& error)
	{
		result.Diagnostics.push_back({ jsonPath.string(), error.what() });
		return result;
	}
	if (!importedScene.parsed)
	{
		result.Diagnostics.push_back({ jsonPath.string(), "could not parse scene JSON" });
		return result;
	}
	RuntimeConversionResult converted = ConvertRuntimeScene(importedScene);
	result.Diagnostics = std::move(converted.Diagnostics);
	result.Data = std::move(converted.Scene);
	return result;
}
