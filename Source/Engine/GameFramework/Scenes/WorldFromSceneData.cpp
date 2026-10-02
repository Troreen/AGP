#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/AssetHandling/TextureAsset.h"
#include "GameFramework/Components/AnimatorComponent.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GameFramework/Scenes/WorldFromSceneData.h"
#include "GameFramework/World/World.h"

#include <GraphicsEngine/GraphicsEngine.h>
#include <Logger/Logger.h>
#include <sstream>
#include <stdexcept>
#include <type_traits>

DECLARE_LOG_CATEGORY_WITH_NAME(WorldParser, WORLD_PARSER, Verbose);

DEFINE_LOG_CATEGORY(WorldParser);

bool WorldFromSceneConverter::Initialize(const std::filesystem::path& aNameConversions)
{
	simdjson::padded_string json = simdjson::padded_string::load(aNameConversions.c_str());
	simdjson::dom::parser parser;
	simdjson::dom::object root;

	simdjson::error_code error = parser.parse(json).get(root);
	if (error != simdjson::error_code::SUCCESS)
	{
		LOG(WorldParser, Warning, "Could not parse name conversion json file '{}'.", aNameConversions.string());
		return false;
	}

	simdjson::dom::object baseNames = root["UnrealBaseNames"];
	for (auto it = baseNames.begin(); it != baseNames.end(); ++it)
	{
		simdjson::dom::key_value_pair itValue = *it;
		const std::string unrealName = std::string(itValue.key);
		const std::string materialName = std::string(itValue.value.get_string().value());

		myUnrealDataNameToMaterialParameterName[unrealName] = materialName;
	}

	simdjson::dom::object textureNames = root["TextureParameterNamesToSlot"];
	for (auto it = textureNames.begin(); it != textureNames.end(); ++it)
	{
		simdjson::dom::key_value_pair itValue = *it;
		const std::string unrealName = std::string(itValue.key);
		const unsigned textureSlot = static_cast<unsigned>(itValue.value.get_uint64().value());

		myUnrealParameterNameToTextureSlot[unrealName] = textureSlot;
	}

	return true;
}

std::unique_ptr<World> WorldFromSceneConverter::BuildWorldFromSceneData(const SceneData& aScene, AssetRegistry& anAssetRegistry, const std::filesystem::path& aShaderPath, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks)
{
	std::unique_ptr<World> world = std::make_unique<World>();
	CameraComponent* selectedCamera = nullptr;
	std::vector<std::string> diagnostics;

	GraphicsEngine& graphicsEngine = GraphicsEngine::Get();

	for (const MasterMaterialRecord& materialData : aScene.Materials)
	{
		MaterialDescription matDesc;
		matDesc.Name = materialData.Name;
		matDesc.Domain = static_cast<MaterialDomain>(materialData.Domain + 1);
		matDesc.ShadingModel = static_cast<ShadingModel>(materialData.ShadingModel + 1);
		switch (materialData.BlendMode)
		{
			case 0:
			default:
				matDesc.BlendMode = BlendMode::Opaque;
				break;

			case 1:
				matDesc.BlendMode = BlendMode::Alpha;
				break;

			case 3:
				matDesc.BlendMode = BlendMode::Additive;
				break;
		}
		matDesc.CullMode = materialData.TwoSided ? RasterizerCullMode::None : RasterizerCullMode::Back;

		const std::filesystem::path shaderFile = aShaderPath / (matDesc.Name + ".hlsli");
		if (std::filesystem::exists(shaderFile))
		{
			matDesc.MaterialShaderCode = shaderFile;
		}
		else
		{
			matDesc.MaterialShaderCode = someFallbacks.MissingShader;
		}

		std::shared_ptr<Material> material = std::make_shared<Material>();
		if (!graphicsEngine.CreateMaterial(matDesc, *material))
		{
			LOG(WorldParser, Warning, "Could not create material with name {}.", matDesc.Name);
			continue;
		}

		std::shared_ptr<MaterialInstance> instance = MaterialInstance::Create(matDesc.Name + "_Inst", material);

		{
			const std::string baseColorName = myUnrealDataNameToMaterialParameterName["Base Color"];
			const std::string metallicName = myUnrealDataNameToMaterialParameterName["Metallic"];
			const std::string roughnessName = myUnrealDataNameToMaterialParameterName["Roughness"];
			instance->SetValue(baseColorName, materialData.BaseColor);
			instance->SetValue(metallicName, materialData.Metallic);
			instance->SetValue(roughnessName, materialData.Roughness);
		}

		for (const MaterialParameterData& parameterData : materialData.Parameters)
		{
			const std::string materialParameterName = parameterData.Name.starts_with("MB_") ? parameterData.Name : ("MB_" + parameterData.Name);

			const float* scalar = std::get_if<float>(&parameterData.Value);
			const CommonUtilities::Vector4f* vector = std::get_if<CommonUtilities::Vector4f>(&parameterData.Value);
			const std::string* string = std::get_if<std::string>(&parameterData.Value);

			if (scalar != nullptr)
			{
				instance->SetValue(materialParameterName, *scalar);
			}
			else if (vector != nullptr)
			{
				instance->SetValue(materialParameterName, *vector);
			}
			else if (string != nullptr && myUnrealParameterNameToTextureSlot.contains(parameterData.Name))
			{
				std::shared_ptr<TextureAsset> texture = anAssetRegistry.GetAsset<TextureAsset>(*string);
				if (texture != nullptr && texture->GetTextureShared() != nullptr)
				{
					instance->SetTexture(myUnrealParameterNameToTextureSlot[parameterData.Name], texture->GetTextureShared());
				}
			}
		}

		std::shared_ptr<MaterialAsset> materialAsset = std::make_shared<MaterialAsset>(instance);
		anAssetRegistry.Pin(matDesc.Name, materialAsset);
	}

	for (const ActorRecord& actorData : aScene.Actors)
	{
		Actor* actor = nullptr;
		try
		{
			actor = world->SpawnActor(actorData.Name);
		}
		catch (const std::exception& error)
		{
			diagnostics.push_back(actorData.Name + ": " + error.what());
			continue;
		}

		actor->SetSourceMetadata(actorData.Archetype, actorData.Tags);
		actor->SetActive(actorData.Active);
		
		if (!actor->GetTransform().SetData(actorData.Transform))
		{
			diagnostics.push_back(actorData.Name + ": Invalid Actor transform");
		}

		for (const ComponentRecord& record : actorData.Components)
		{
			const ComponentData& common = Common(record);
			try
			{
				Component* created = CreateComponent(*actor, record, anAssetRegistry, aClientSize, someFallbacks);
				if (created == nullptr)
				{
					continue;
				}

				ApplyComponentData(*created, common);

				// If this component is a camera, remember it so we can set it as the active camera for the world.
				if (std::holds_alternative<CameraData>(record))
				{
					selectedCamera = static_cast<CameraComponent*>(created);
				}
			}
			catch (const std::exception& error)
			{
				diagnostics.push_back(actorData.Name + "/" + common.Name + ": " + error.what());
			}
		}
	}
	if (!diagnostics.empty())
	{
		std::ostringstream message;
		message << "Scene construction failed:";
		for (const std::string& diagnostic : diagnostics)
		{
			message << "\n - " << diagnostic;
		}
		throw std::runtime_error(message.str());
	}
	if (selectedCamera)
	{
		world->SetActiveCamera(selectedCamera);
	}
	return world;
}

const ComponentData& WorldFromSceneConverter::Common(const ComponentRecord& aRecord)
{
	return std::visit(
		[](const auto& aValue)
		-> const ComponentData&
		{
			return aValue.Common;
		},
		aRecord);
}

void WorldFromSceneConverter::ApplyComponentData(Component& aComponent, const ComponentData& someData)
{
	aComponent.SetSourceMetadata(someData.Tags, someData.SourceParent);
	aComponent.SetEnabled(someData.Enabled);

	SceneComponent* sceneComponent = dynamic_cast<SceneComponent*>(&aComponent);
	if (sceneComponent && !sceneComponent->GetTransform().SetData(someData.Transform))
	{
		throw std::runtime_error("Invalid component transform");
	}
}

bool WorldFromSceneConverter::ApplyMesh(MeshComponentBase& aComponent, const StaticMeshData& someData, AssetRegistry& anAssetRegistry, const SceneFallbackAssets& someFallbacks)
{
	std::shared_ptr<MeshAsset> mesh = anAssetRegistry.GetAsset<MeshAsset>(someData.MeshName);
	if (!mesh)
	{
		GFLOG(Warning, "Using fallback cube for mesh component '{}': {}", someData.Common.Name, anAssetRegistry.GetLastError());
		mesh = someFallbacks.MissingMesh;
		if (!mesh)
		{
			return false;
		}
	}
	const bool usingFallbackMesh = mesh == someFallbacks.MissingMesh;

	aComponent.SetMesh(mesh);
	aComponent.SetVisible(someData.Visible);

	if (!usingFallbackMesh && someData.Materials.size() > aComponent.GetMaterialCount())
	{
		throw std::runtime_error("Material list exceeds mesh slots");
	}

	for (unsigned materialSlot = 0; materialSlot < aComponent.GetMaterialCount(); ++materialSlot)
	{
		std::shared_ptr<MaterialAsset> material;
		if (materialSlot < someData.Materials.size())
		{
			const MaterialInstanceData& materialData = someData.Materials[materialSlot];

			material = anAssetRegistry.GetAsset<MaterialAsset>(materialData.Name);
			if (material == nullptr)
			{
				if (materialData.Parent.empty() || materialData.Parent == "Self")
				{
					GFLOG(Warning, "Cannot load material '{}' for mesh component '{}': {}", materialData.Name, someData.Common.Name, anAssetRegistry.GetLastError());
				}
				else
				{
					std::shared_ptr<MaterialAsset> parentMaterial = anAssetRegistry.GetAsset<MaterialAsset>(materialData.Parent);
					if (parentMaterial == nullptr)
					{
						GFLOG(Warning, "Parent material {} cannot be loaded for material '{}' for mesh component '{}': {}", materialData.Parent, materialData.Name, someData.Common.Name, anAssetRegistry.GetLastError());
					}
					else
					{
						std::shared_ptr<MaterialInstance> instance = MaterialInstance::Create(materialData.Name, parentMaterial->GetMaterial());

						for (const MaterialParameterData& parameterData : materialData.Parameters)
						{
							std::string materialParameterName = parameterData.Name.starts_with("MB_") ? parameterData.Name : ("MB_" + parameterData.Name);
							for (size_t nameIndex = 0; nameIndex < materialParameterName.size(); ++nameIndex)
							{
								if (materialParameterName[nameIndex] == ' ')
								{
									materialParameterName[nameIndex] = '_';
								}
							}

							const float* scalar = std::get_if<float>(&parameterData.Value);
							const CommonUtilities::Vector4f* vector = std::get_if<CommonUtilities::Vector4f>(&parameterData.Value);
							const std::string* string = std::get_if<std::string>(&parameterData.Value);

							if (scalar != nullptr)
							{
								instance->SetValue(materialParameterName, *scalar);
							}
							else if (vector != nullptr)
							{
								instance->SetValue(materialParameterName, *vector);
							}
							else if (string != nullptr && myUnrealParameterNameToTextureSlot.contains(parameterData.Name))
							{
								std::shared_ptr<TextureAsset> texture = anAssetRegistry.GetAsset<TextureAsset>(*string);
								if (texture != nullptr && texture->GetTextureShared() != nullptr)
								{
									instance->SetTexture(myUnrealParameterNameToTextureSlot[parameterData.Name], texture->GetTextureShared());
								}
							}
						}

						material = std::make_shared<MaterialAsset>(instance);
						anAssetRegistry.Pin(materialData.Name, material);
					}
				}
			}
		}

		if (!material)
		{
			// TODO: Bind a dedicated error texture/material when those assets are available.
			if (!usingFallbackMesh)
			{
				GFLOG(Warning, "Using fallback material for mesh component '{}' at slot {}.", someData.Common.Name, materialSlot);
			}
			material = someFallbacks.MissingMaterial;
			if (!material)
			{
				return false;
			}
		}

		if (!aComponent.SetMaterial(materialSlot, material))
		{
			throw std::runtime_error("Material slot is invalid: " + std::to_string(materialSlot));
		}
	}
	return true;
}

Component* WorldFromSceneConverter::CreateComponent(Actor& anActor, const ComponentRecord& aRecord, AssetRegistry& anAssetRegistry, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks)
{
	const ComponentData& common = Common(aRecord);
	// Create the component that matches the data stored in this record.
	return std::visit([&](const auto& someComponentData) -> Component*
		{
			using ComponentType = std::decay_t<decltype(someComponentData)>;
			if constexpr (std::is_same_v<ComponentType, SceneComponentData>)
			{
				return anActor.AddComponent<SceneComponent>(common.Name);
			}
			else if constexpr (std::is_same_v<ComponentType, CameraData>)
			{
				return anActor.AddComponent<CameraComponent>(common.Name, someComponentData.FieldOfView,
					someComponentData.NearPlane, someComponentData.FarPlane, aClientSize);
			}
			else if constexpr (std::is_same_v<ComponentType, StaticMeshData>)
			{
				StaticMeshComponent* meshComponent = anActor.AddComponent<StaticMeshComponent>(common.Name);
				if (!ApplyMesh(*meshComponent, someComponentData, anAssetRegistry, someFallbacks))
				{
					// No usable mesh or material was available, so discard this component.
					meshComponent->Destroy();
					return nullptr;
				}
				return meshComponent;
			}
			else if constexpr (std::is_same_v<ComponentType, SkeletalMeshData>)
			{
				// TODO: Load animator component from Unreal as well
				SkeletalMeshComponent* meshComponent = anActor.AddComponent<SkeletalMeshComponent>(common.Name);
				AnimatorComponent* animatorComponent = anActor.AddComponent<AnimatorComponent>(common.Name + "Anim");
				animatorComponent->SetMeshComponent(meshComponent);

				if (!ApplyMesh(*meshComponent, someComponentData, anAssetRegistry, someFallbacks))
				{
					meshComponent->Destroy();
					return nullptr;
				}
				if (meshComponent->GetMesh() == someFallbacks.MissingMesh)
				{
					// The fallback cube has no skeleton to animate.
					return meshComponent;
				}
				if (!someComponentData.PartialRoot.empty() &&
					!animatorComponent->ConfigurePartialLayerFromJointName(someComponentData.PartialRoot))
				{
					throw std::runtime_error("Invalid partial root");
				}
				if (!someComponentData.InitialAnimation.empty() &&
					!animatorComponent->PlayAnimation(someComponentData.InitialAnimation, someComponentData.Loop))
				{
					throw std::runtime_error("Invalid animation");
				}
				return meshComponent;
			}
			else if constexpr (std::is_same_v<ComponentType, DirectionalLightData>)
			{
				DirectionalLightComponent* light = anActor.AddComponent<DirectionalLightComponent>(common.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, PointLightData>)
			{
				PointLightComponent* light = anActor.AddComponent<PointLightComponent>(common.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				light->SetRadius(someComponentData.Radius);
				light->SetFalloffExponent(someComponentData.FalloffExponent);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, SpotLightData>)
			{
				SpotLightComponent* light = anActor.AddComponent<SpotLightComponent>(common.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				light->SetRadius(someComponentData.Radius);
				light->SetFalloffExponent(someComponentData.FalloffExponent);
				light->SetConeAnglesDegrees(someComponentData.InnerConeDegrees, someComponentData.OuterConeDegrees);
				return light;
			}
			else
			{
				// Preserve component types that do not have a runtime implementation yet.
				return anActor.AddComponent<PlaceholderComponent>(common.Name, someComponentData.Type, someComponentData.Properties);
			}
		}, aRecord);
}
