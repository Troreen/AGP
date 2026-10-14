#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/AssetHandling/TextureAsset.h"
#include "GameFramework/Components/AnimatorComponent.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/CustomComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GameFramework/Scenes/SceneWorldBuilder.h"
#include "GameFramework/World/World.h"

#include <GraphicsEngine/GraphicsEngine.h>
#include <Logger/Logger.h>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <algorithm>

DECLARE_LOG_CATEGORY_WITH_NAME(WorldParser, WORLD_PARSER, Verbose);

DEFINE_LOG_CATEGORY(WorldParser);

bool SceneWorldBuilder::Initialize(const std::filesystem::path& aNameConversions)
{
	// Load material parameter names and texture slots.
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

std::unique_ptr<World> SceneWorldBuilder::BuildWorld(const SceneData& aScene, AssetRegistry& anAssetRegistry, const std::filesystem::path& aShaderPath, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks, const CustomComponentCreator& aCustomCreator)
{
	std::unique_ptr<World> world = std::make_unique<World>();
	CreateSceneMaterials(aScene.Materials, anAssetRegistry, aShaderPath, someFallbacks);
	CreateActorsAndComponents(*world, aScene.Actors, anAssetRegistry, aClientSize, someFallbacks, aCustomCreator);
	return world;
}

void SceneWorldBuilder::CreateSceneMaterials(const std::vector<MasterMaterialRecord>& someMaterials, AssetRegistry& anAssetRegistry, const std::filesystem::path& aShaderPath, const SceneFallbackAssets& someFallbacks)
{
	GraphicsEngine& graphicsEngine = GraphicsEngine::Get();

	// Create and register the scene's master materials.
	for (const MasterMaterialRecord& materialData : someMaterials)
	{
		MaterialDescription matDesc;
		matDesc.Name = materialData.Name;
		if (materialData.Domain != 0)
		{
			LOG(WorldParser, Warning, "Material '{}' skipped: unsupported Unreal material domain {}. Scene loading will continue.", materialData.Name, materialData.Domain);
			continue;
		}
		matDesc.Domain = MaterialDomain::Surface;
		if (materialData.ShadingModel != 0 && materialData.ShadingModel != 1)
		{
			LOG(WorldParser, Warning, "Material '{}' skipped: unsupported Unreal shading model {}. Scene loading will continue.", materialData.Name, materialData.ShadingModel);
			continue;
		}
		matDesc.ShadingModel = materialData.ShadingModel == 0 ? ShadingModel::Unlit : ShadingModel::Lit;
		switch (materialData.BlendMode)
		{
			case 0:
				matDesc.BlendMode = BlendMode::Opaque;
				break;

			case 1:
				matDesc.BlendMode = BlendMode::Alpha;
				break;

			case 3:
				matDesc.BlendMode = BlendMode::Additive;
				break;

			default:
				LOG(WorldParser, Warning, "Material '{}' skipped: unsupported Unreal blend mode {}. Scene loading will continue.", materialData.Name, materialData.BlendMode);
				continue;
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

		// Apply base color, metallic, and roughness.
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
				if (texture != nullptr && texture->GetTexture() != nullptr)
				{
					instance->SetTexture(myUnrealParameterNameToTextureSlot[parameterData.Name], texture->GetTexture());
				}
			}
		}

		std::shared_ptr<MaterialAsset> materialAsset = std::make_shared<MaterialAsset>(instance);
		anAssetRegistry.Pin(matDesc.Name, materialAsset);
	}
}

void SceneWorldBuilder::CreateActorsAndComponents(World& aWorld, const std::vector<ActorRecord>& someActors, AssetRegistry& anAssetRegistry, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks, const CustomComponentCreator& aCustomCreator)
{
	CameraComponent* selectedCamera = nullptr;
	std::vector<std::string> diagnostics;

	// Create actors and their components.
	for (const ActorRecord& actorData : someActors)
	{
		Actor* actor = nullptr;
		try
		{
			actor = aWorld.SpawnActor(actorData.Name);
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

		for (const ComponentRecord& componentRecord : actorData.Components)
		{
			const ComponentData& commonData = GetCommonComponentData(componentRecord);
			try
			{
				// Create custom components through the supplied callback.
				const auto* customComponentData = std::get_if<CustomComponentData>(&componentRecord);
				if (customComponentData)
				{
					if (!aCustomCreator)
					{
						throw std::runtime_error("No custom component creator supplied for " + customComponentData->CustomType);
					}
					CustomComponent* customComponent = aCustomCreator(*actor, *customComponentData);
					if (customComponent == nullptr)
					{
						continue;
					}

					// Apply common data, then custom variables.
					ApplyCommonComponentData(*customComponent, customComponentData->Common);
					customComponent->ApplyCustomVariables(customComponentData->Variables);
					continue;
				}

				Component* created = CreateComponent(*actor, componentRecord, anAssetRegistry, someFallbacks);
				if (created == nullptr)
				{
					continue;
				}

				ApplyCommonComponentData(*created, commonData);

				// Select this camera and set its projection.
				if (std::holds_alternative<CameraData>(componentRecord))
				{
					selectedCamera = static_cast<CameraComponent*>(created);
					const auto& cameraData = std::get<CameraData>(componentRecord);
					if (!selectedCamera->SetPerspective(cameraData.FieldOfView, cameraData.NearPlane, cameraData.FarPlane, aClientSize))
					{
						throw std::runtime_error("Invalid camera projection");
					}
				}
			}
			catch (const std::exception& error)
			{
				diagnostics.push_back(actorData.Name + "/" + commonData.Name + ": " + error.what());
			}
		}
	}
	// Throw one error containing all construction errors.
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
		aWorld.SetActiveCamera(selectedCamera);
	}
}

const ComponentData& SceneWorldBuilder::GetCommonComponentData(const ComponentRecord& aRecord)
{
	return std::visit(
		[](const auto& aValue)
		-> const ComponentData&
		{
			return aValue.Common;
		},
		aRecord);
}

void SceneWorldBuilder::ApplyCommonComponentData(Component& aComponent, const ComponentData& someComponentData)
{
	aComponent.SetSourceMetadata(someComponentData.Tags, someComponentData.SourceParent);
	aComponent.SetEnabled(someComponentData.Enabled);

	// Apply the transform to scene components.
	SceneComponent* sceneComponent = dynamic_cast<SceneComponent*>(&aComponent);
	if (sceneComponent && !sceneComponent->GetTransform().SetData(someComponentData.Transform))
	{
		throw std::runtime_error("Invalid component transform");
	}
}

std::shared_ptr<MaterialAsset> SceneWorldBuilder::LoadMaterial(const MaterialInstanceData& someMaterialData, AssetRegistry& anAssetRegistry, const std::string& aComponentName)
{
	// Load the material, or create an instance from its parent.
	std::shared_ptr<MaterialAsset> material = anAssetRegistry.GetAsset<MaterialAsset>(someMaterialData.Name);
	if (material)
	{
		return material;
	}
	if (someMaterialData.Parent.empty() || someMaterialData.Parent == "Self")
	{
		GFLOG(Warning, "Cannot load material '{}' for mesh component '{}': {}", someMaterialData.Name, aComponentName, anAssetRegistry.GetLastError());
		return nullptr;
	}
	std::shared_ptr<MaterialAsset> parentMaterial = anAssetRegistry.GetAsset<MaterialAsset>(someMaterialData.Parent);
	if (!parentMaterial)
	{
		GFLOG(Warning, "Parent material {} cannot be loaded for material '{}' for mesh component '{}': {}", someMaterialData.Parent, someMaterialData.Name, aComponentName, anAssetRegistry.GetLastError());
		return nullptr;
	}

	std::shared_ptr<MaterialInstance> instance = MaterialInstance::Create(someMaterialData.Name, parentMaterial->GetMaterial());
	for (const MaterialParameterData& parameterData : someMaterialData.Parameters)
	{
		// TODO: Check whether we still need custom material shaders. If we only use the default shader, apply the values it supports instead of trying every Unreal parameter.
		// Add MB_ and replace spaces for scalar and vector parameter names.
		std::string materialParameterName = parameterData.Name.starts_with("MB_") ? parameterData.Name : "MB_" + parameterData.Name;
		std::replace(materialParameterName.begin(), materialParameterName.end(), ' ', '_');

		const float* scalar = std::get_if<float>(&parameterData.Value);
		const CommonUtilities::Vector4f* vector = std::get_if<CommonUtilities::Vector4f>(&parameterData.Value);
		const std::string* string = std::get_if<std::string>(&parameterData.Value);

		if (scalar)
		{
			instance->SetValue(materialParameterName, *scalar);
		}
		else if (vector)
		{
			instance->SetValue(materialParameterName, *vector);
		}
		else if (string && myUnrealParameterNameToTextureSlot.contains(parameterData.Name))
		{
			std::shared_ptr<TextureAsset> texture = anAssetRegistry.GetAsset<TextureAsset>(*string);
			if (texture && texture->GetTexture())
			{
				instance->SetTexture(myUnrealParameterNameToTextureSlot[parameterData.Name], texture->GetTexture());
			}
		}
	}

	material = std::make_shared<MaterialAsset>(instance);
	anAssetRegistry.Pin(someMaterialData.Name, material);
	return material;
}

bool SceneWorldBuilder::ApplyMesh(Actor& anActor, MeshComponentBase& aComponent, const StaticMeshData& someMeshData, AssetRegistry& anAssetRegistry, const SceneFallbackAssets& someFallbacks)
{
	std::shared_ptr<MeshAsset> mesh = anAssetRegistry.GetAsset<MeshAsset>(someMeshData.MeshName);
	if (!mesh)
	{
		GFLOG(Warning, "Using fallback cube for mesh component '{}' on actor '{}': {}", someMeshData.Common.Name, anActor.GetName(), anAssetRegistry.GetLastError());
		mesh = someFallbacks.MissingMesh;
		if (!mesh)
		{
			return false;
		}
	}
	const bool usingFallbackMesh = mesh == someFallbacks.MissingMesh;

	aComponent.SetMesh(mesh);
	aComponent.SetVisible(someMeshData.Visible);

	if (!usingFallbackMesh && someMeshData.Materials.size() > aComponent.GetMaterialCount())
	{
		throw std::runtime_error("Material list exceeds mesh slots");
	}

	// Assign a material or fallback to each mesh slot.
	for (unsigned materialSlot = 0; materialSlot < aComponent.GetMaterialCount(); ++materialSlot)
	{
		std::shared_ptr<MaterialAsset> material;
		if (materialSlot < someMeshData.Materials.size())
		{
			material = LoadMaterial(someMeshData.Materials[materialSlot], anAssetRegistry, someMeshData.Common.Name);
		}

		if (!material)
		{
			if (!usingFallbackMesh)
			{
				GFLOG(Warning, "Using fallback material for mesh component '{}' on actor '{}' at slot {}.", someMeshData.Common.Name, anActor.GetName(), materialSlot);
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

Component* SceneWorldBuilder::CreateComponent(Actor& anActor, const ComponentRecord& aRecord, AssetRegistry& anAssetRegistry, const SceneFallbackAssets& someFallbacks)
{
	const ComponentData& commonData = GetCommonComponentData(aRecord);
	// Create the component that matches the data stored in this record.
	return std::visit([&](const auto& someComponentData) -> Component*
		{
			using ComponentType = std::decay_t<decltype(someComponentData)>;
			if constexpr (std::is_same_v<ComponentType, SceneComponentData>)
			{
				return anActor.AddComponent<SceneComponent>(commonData.Name);
			}
			else if constexpr (std::is_same_v<ComponentType, CameraData>)
			{
				return anActor.AddComponent<CameraComponent>(commonData.Name);
			}
			else if constexpr (std::is_same_v<ComponentType, StaticMeshData>)
			{
				StaticMeshComponent* meshComponent = anActor.AddComponent<StaticMeshComponent>(commonData.Name);
				if (!ApplyMesh(anActor, *meshComponent, someComponentData, anAssetRegistry, someFallbacks))
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
				SkeletalMeshComponent* meshComponent = anActor.AddComponent<SkeletalMeshComponent>(commonData.Name);
				AnimatorComponent* animatorComponent = anActor.GetComponent<AnimatorComponent>();

				if (!ApplyMesh(anActor, *meshComponent, someComponentData, anAssetRegistry, someFallbacks))
				{
					meshComponent->Destroy();
					return nullptr;
				}
				if (meshComponent->GetMesh() == someFallbacks.MissingMesh)
				{
					// The fallback cube has no skeleton to animate.
					return meshComponent;
				}

				if (animatorComponent != nullptr)
				{
					animatorComponent->SetMeshComponent(meshComponent);
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
				}

				return meshComponent;
			}
			else if constexpr (std::is_same_v<ComponentType, DirectionalLightData>)
			{
				DirectionalLightComponent* light = anActor.AddComponent<DirectionalLightComponent>(commonData.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, PointLightData>)
			{
				PointLightComponent* light = anActor.AddComponent<PointLightComponent>(commonData.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				light->SetRadius(someComponentData.Radius);
				light->SetFalloffExponent(someComponentData.FalloffExponent);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, SpotLightData>)
			{
				SpotLightComponent* light = anActor.AddComponent<SpotLightComponent>(commonData.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				light->SetRadius(someComponentData.Radius);
				light->SetFalloffExponent(someComponentData.FalloffExponent);
				light->SetConeAnglesDegrees(someComponentData.InnerConeDegrees, someComponentData.OuterConeDegrees);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, CustomComponentData>)
			{
				// This function only creates built-in types.
				// CreateActorsAndComponents asks game code to create custom types.
				return nullptr;
			}
			else
			{
				// Preserve component types that do not have a runtime implementation yet.
				return anActor.AddComponent<PlaceholderComponent>(commonData.Name, someComponentData.Type, someComponentData.Properties);
			}
		}, aRecord);
}
