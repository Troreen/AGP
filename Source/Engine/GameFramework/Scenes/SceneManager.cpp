#include "GameFramework.pch.h"
#include "SceneManager.h"

#include "AssetHandling/AssetRegistry.h"
#include "AssetHandling/MaterialAsset.h"
#include "AssetHandling/MeshAsset.h"
#include "AssetHandling/NavMeshAsset.h"
#include "AssetHandling/SceneAsset.h"
#include "AssetHandling/TextureAsset.h"
#include "Helper/JsonHelper.hpp"
#include "SceneData.h"
#include "SceneWorldBuilder.h"
#include "ServiceLocator.h"
#include "Settings/EngineSettings.h"

#include <AISystems/Pathfinder.hpp>
#include <Logger/Logger.h>
#include <GraphicsEngine/GraphicsEngine.h>
#include <GraphicsEngine/Materials/Material.h>

DECLARE_LOG_CATEGORY_WITH_NAME(LogSceneManager, Scene_Manager, Verbose);

DEFINE_LOG_CATEGORY(LogSceneManager);

SceneManager::SceneManager() : myIsDisabled(false), myRequestedScene(nullptr), myCurrentScene(nullptr)
{
}

SceneManager::~SceneManager() = default;

void SceneManager::Initialize(const std::filesystem::path& aContentRoot)
{
	AssetRegistry& assets = ServiceLocator::GetInstance().GetAssetRegistry();
	myFallbackAssets.MissingMesh = assets.GetAsset<MeshAsset>("SM_Cube.fbx");
	myFallbackAssets.MissingShader = aContentRoot / "Shaders" / "_DefaultMaterial.hlsli";

	MaterialDescription matDesc;
	matDesc.Name = "_Default Material_";
	matDesc.Domain = MaterialDomain::Surface;
	matDesc.ShadingModel = ShadingModel::Lit;
	matDesc.BlendMode = BlendMode::Opaque;
	matDesc.MaterialShaderCode = myFallbackAssets.MissingShader;
	matDesc.AlbedoTexture = aContentRoot / "Textures" / "T_ERROR_FALLBACK_BaseColor.dds";

	std::shared_ptr<Material> material = std::make_shared<Material>();
	if (GraphicsEngine::Get().CreateMaterial(matDesc, *material))
	{
		myFallbackAssets.MissingMaterial = std::make_shared<MaterialAsset>(material);
	}
	else
	{
		LOG(LogSceneManager, Warning, "Could not create default material!");
	}

	myIsDisabled = false;
	myRequestedScene = nullptr;
	myCurrentScene = nullptr;
}

bool SceneManager::LoadScene(const std::shared_ptr<SceneAsset>& aScene)
{
	if (myIsDisabled)
	{
		return false;
	}

	myRequestedScene = aScene;
	return true;
}

bool SceneManager::ReloadCurrentScene()
{
	return myCurrentScene != nullptr && LoadScene(myCurrentScene);
}

bool SceneManager::ProcessSceneLoadRequest()
{
	if (myRequestedScene == nullptr || myIsDisabled)
	{
		return false;
	}

	AssetRegistry& assets = ServiceLocator::GetInstance().GetAssetRegistry();
	const std::filesystem::path& shaderFolder = ServiceLocator::GetInstance().GetEngineSettings().GetApplicationSettings().ShaderFolder;
	const std::shared_ptr<SceneData>& requestedData = myRequestedScene->GetSceneData();

	SceneWorldBuilder worldBuilder;
	std::unique_ptr<World> candidateWorld = worldBuilder.BuildWorld(*requestedData, assets, shaderFolder, myFallbackAssets,
		[this](Actor& anActor, const CustomComponentData& aCustomComponentData)
		{
			CustomComponent* component = myCustomComponentRegistry.Create(anActor, aCustomComponentData);
			if (!component)
			{
				LOG(LogSceneManager, Warning, "Skipping unknown custom component '{}' on actor '{}': {}", aCustomComponentData.Common.Name, anActor.GetName(),
					aCustomComponentData.CustomType);
			}
			return component;
		});

	if (candidateWorld != nullptr)
	{
		if (myCurrentWorld != nullptr)
		{
			myCurrentWorld->Clear();
		}
		myCurrentWorld = std::move(candidateWorld);

		Pathfinder& pathFinder = ServiceLocator::GetInstance().GetPathFinder();
		pathFinder.Clear();

		std::shared_ptr<NavMeshAsset> navMesh = assets.GetAsset<NavMeshAsset>(myRequestedScene->GetSceneName() + ".nav");
		if (navMesh != nullptr)
		{
			pathFinder.Init(navMesh->MoveVertexIndices(), navMesh->MoveVertices());
		}

		myCurrentWorld->BeginPlay();
	}

	AmbientSettings ambientSettings;
	std::shared_ptr<TextureAsset> environmentTexture = assets.GetAsset<TextureAsset>(requestedData->EnvironmentCubeTexture);
	if (environmentTexture != nullptr)
	{
		ambientSettings.EnvironmentCubeTexture = *environmentTexture->GetTexture();
	}
	ambientSettings.AmbientLightColor = requestedData->AmbientLightColor;
	ambientSettings.AmbientLightIntensity = requestedData->AmbientLightIntensity;
	GraphicsEngine::Get().SetAmbientSettings(std::move(ambientSettings));

	myCurrentScene = std::move(myRequestedScene);
	myRequestedScene = nullptr;

	myLoadSceneCallback.Invoke();

	return true;
}

void SceneManager::Cleanup()
{
	if (myCurrentWorld != nullptr)
	{
		myCurrentWorld->Clear();
	}
	myIsDisabled = true;
}

void SceneManager::TryLoadAmbientSettings(const std::filesystem::path& aSettingsFile, AmbientSettings& inoutAmbientSettings)
{
	if (!std::filesystem::exists(aSettingsFile))
	{
		return;
	}

	simdjson::padded_string json = simdjson::padded_string::load(aSettingsFile.c_str());
	simdjson::dom::parser parser;
	JsonObject root;
	const simdjson::error_code& error = parser.parse(json).get(root);
	if (error)
	{
		return;
	}

	std::string textureName{};
	if (TMJson::GetElement_s(root, "EnvironmentCubeTexture", textureName))
	{
		std::shared_ptr<TextureAsset> environmentTexture = ServiceLocator::GetInstance().GetAssetRegistry().GetAsset<TextureAsset>(textureName);
		if (environmentTexture != nullptr)
		{
			inoutAmbientSettings.EnvironmentCubeTexture = *environmentTexture->GetTexture();
		}
	}

	CU::Vector3f ambientColor{};
	if (TMJson::GetElement_s(root, "AmbientLightColor", ambientColor))
	{
		inoutAmbientSettings.AmbientLightColor = ambientColor;
	}

	float ambientIntensity{};
	if (TMJson::GetElement_s(root, "AmbientLightIntensity", ambientIntensity))
	{
		inoutAmbientSettings.AmbientLightIntensity = ambientIntensity;
	}

	bool showSkybox{};
	if (TMJson::GetElement_s(root, "ShowSkybox", showSkybox))
	{
		inoutAmbientSettings.ShowSkybox = showSkybox;
	}
}
