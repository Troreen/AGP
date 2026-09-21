#pragma once

#include "Asset.h"

#include <memory>

class Font;
class TextureAsset;

class FontAsset : public Asset
{
public:
	FontAsset() = default;
	explicit FontAsset(std::shared_ptr<Font> font) : myFont(std::move(font)) {}
	const std::shared_ptr<Font>& GetFont() const { return myFont; }

protected:
	bool Load(const std::filesystem::path& path, AssetRegistry& registry) override;

private:
	std::shared_ptr<Font> myFont;
	std::shared_ptr<TextureAsset> myAtlasAsset;
};
