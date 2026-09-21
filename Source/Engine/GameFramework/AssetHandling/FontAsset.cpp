#include "FontAsset.h"
#include "AssetRegistry.h"
#include "TextureAsset.h"

bool FontAsset::Load(const std::filesystem::path& path, AssetRegistry& registry)
{
	myFont = registry.LoadFont(path, myAtlasAsset);
	return bool(myFont);
}
