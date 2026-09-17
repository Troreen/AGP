#pragma once
#include <filesystem>
#include <unordered_map>
#include "MeshLibrary.h"
#include "GameFramework/Scenes/SceneBuilder.h"
class GameContext;
class MaterialInterface;

// Game-owned scene authoring and the temporary synchronous asset adapter. Worlds
// are owned by the host; this helper can safely survive any number of scene reloads.
class ModelViewerScene final
{
public:
    void Initialize(GameContext& context);
    void Reload(GameContext& context);
private:
    SceneBuildResult Build(const GameInput* input, CommonUtilities::Vector2u resolution);
    std::shared_ptr<MaterialInterface> GetMaterial(const std::filesystem::path& file);
    MeshLibrary myMeshLibrary;
    ComponentRegistry myRegistry;
    std::filesystem::path myContentRoot;
    std::unordered_map<std::string,std::shared_ptr<MaterialInterface>> myMaterialCache;
};
