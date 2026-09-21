#include "TextureAsset.h"

#include <GraphicsEngine/GraphicsEngine.h>
#include <Logger/Logger.h>

DECLARE_LOG_CATEGORY_WITH_NAME(TextureAssetLog, ASSET, Verbose);

DEFINE_LOG_CATEGORY(TextureAssetLog);

TextureAsset::TextureAsset() = default;

TextureAsset::TextureAsset(std::shared_ptr<Texture> aTexture) : myTexture(std::move(aTexture))
{
}

TextureAsset::~TextureAsset() = default;

const Texture* TextureAsset::GetTexture() const
{
    return myTexture.get();
}

bool TextureAsset::Load(const std::filesystem::path& aPath, [[maybe_unused]] AssetRegistry& aRegistry)
{
    std::shared_ptr<Texture> texture = std::make_shared<Texture>();
    if (!GraphicsEngine::Get().LoadTexture(aPath, *texture))
    {
        LOG(TextureAssetLog, Warning, "Could not load texture asset {}", aPath.string());
        return false;
    }

    myTexture = std::move(texture);
    return true;
}
