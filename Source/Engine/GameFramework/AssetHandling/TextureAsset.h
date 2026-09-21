#pragma once

#include "Asset.h"

#include <GraphicsEngine/Objects/Texture.h>

class TextureAsset : public Asset
{
public:
    TextureAsset();
    TextureAsset(std::shared_ptr<Texture> aTexture);
    ~TextureAsset() override;

    const Texture* GetTexture() const;
    const std::shared_ptr<Texture>& GetTextureShared() const { return myTexture; }

protected:
    bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) override;

private:
    std::shared_ptr<Texture> myTexture;
};
