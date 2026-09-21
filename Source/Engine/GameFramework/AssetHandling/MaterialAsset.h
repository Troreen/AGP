#pragma once

#include "Asset.h"
#include "../SimdJson/simdjson.h"

#include <GraphicsEngine/Materials/Material.h>
#include <vector>

class TextureAsset;

class MaterialAsset : public Asset
{
public:
    MaterialAsset();
    explicit MaterialAsset(std::shared_ptr<MaterialInterface> material);
    ~MaterialAsset() override;

    const std::shared_ptr<MaterialInterface>& GetMaterial() const;

protected:
    bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) override;

private:
    bool CreateMaterial(const std::filesystem::path& aPath, const simdjson::dom::object& aRoot);
    bool CreateMaterialInstance(const std::filesystem::path& aPath, const simdjson::dom::object& aRoot, AssetRegistry& aRegistry);
    bool SetParameters(const simdjson::dom::object& aRoot, std::shared_ptr<MaterialInstance>& aMaterial);

    std::shared_ptr<MaterialInterface> myMaterial;
    std::shared_ptr<MaterialAsset> myParentAsset;
    std::vector<std::shared_ptr<TextureAsset>> myTextureAssets;
};
