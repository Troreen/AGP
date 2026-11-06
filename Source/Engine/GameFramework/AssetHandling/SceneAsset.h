#pragma once

#include "Asset.h"

#include "../Scenes/SceneData.h"

class SceneAsset : public Asset
{
public:
	SceneAsset();
	~SceneAsset() override;

	const std::shared_ptr<SceneData>& GetSceneData() const;
	const std::string& GetSceneName() const;

protected:
	bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) override;

private:
	std::shared_ptr<SceneData> myScene;

	std::string mySceneName;
};
