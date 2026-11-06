#include "GameFramework.pch.h"
#include "SceneAsset.h"

#include "Scenes/SceneManager.h"
#include "ServiceLocator.h"
#include "UnrealSceneImporter/UnrealSceneImporter.h"

DECLARE_LOG_CATEGORY_WITH_NAME(LogSceneAsset, Scene_Asset, Verbose);

DEFINE_LOG_CATEGORY(LogSceneAsset);

SceneAsset::SceneAsset() = default;
SceneAsset::~SceneAsset() = default;

const std::shared_ptr<SceneData>& SceneAsset::GetSceneData() const
{
	return myScene;
}

const std::string& SceneAsset::GetSceneName() const
{
	return mySceneName;
}

bool SceneAsset::Load(const std::filesystem::path& aPath, [[maybe_unused]] AssetRegistry& aRegistry)
{
	UnrealImportResult importResult = UnrealSceneImporter{}.ImportScene(aPath);
	if (!importResult)
	{
		for (const ImportDiagnostic& diagnostic : importResult.Diagnostics)
		{
			LOG(LogSceneAsset, Error, "Scene import error in '{}': {}", diagnostic.Context, diagnostic.Message);
		}
		LOG(LogSceneAsset, Log, "Scene import failed: {}", aPath.string());
		return false;
	}

	mySceneName = aPath.stem().string();
	LOG(LogSceneAsset, Log, "Loaded scene '{}' from '{}' ({} actors).", mySceneName, aPath.string(), importResult.Data->Actors.size());

	myScene = std::make_shared<SceneData>(std::move(*importResult.Data));
	return true;
}
