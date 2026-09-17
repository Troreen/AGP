#pragma once
#include <filesystem>
#include <unordered_map>
#include "MeshLibrary.h"
#include "GameFramework/Integration/ISceneSource.h"
class MaterialInterface;

// Game-owned scene authoring and the temporary synchronous asset adapter. Worlds
// are owned by the host; this helper can safely survive any number of scene reloads.
class ModelViewerScene final : public GameFrameworkIntegration::ISceneSource
{
public:
	GameFrameworkIntegration::SceneSourceResult Load(const SceneId&, GameFrameworkIntegration::SceneLoadContext&) override;

private:
	std::shared_ptr<MaterialInterface> GetMaterial(const std::filesystem::path& file);
	MeshLibrary myMeshLibrary;

	std::filesystem::path myContentRoot;
	bool myInitialized = false;
	std::unordered_map<std::string, std::shared_ptr<MaterialInterface>> myMaterialCache;
};
