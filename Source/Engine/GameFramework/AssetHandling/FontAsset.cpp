#include "FontAsset.h"

#include "AssetRegistry.h"
#include "TextureAsset.h"
#include "../SimdJson/simdjson.h"

#include <GraphicsEngine/Objects/Font.h>
#include <Logger/Logger.h>
#include <Rectangle.h>

DECLARE_LOG_CATEGORY_WITH_NAME(FontAssetLog, ASSET, Verbose);

DEFINE_LOG_CATEGORY(FontAssetLog);

namespace
{
	std::string ReadString(const simdjson::dom::object& object, std::string_view key, std::string fallback = {})
	{
		auto value = object.at_key(key);
		if (value.error() || !value.value().is_string())
		{
			return fallback;
		}
		return std::string(value.value().get_string().value());
	}

	std::filesystem::path ResolveRelative(const std::filesystem::path& parent, const std::string& value)
	{
		if (value.empty()) return {};
		const std::filesystem::path path(value);
		return path.is_absolute() ? path.lexically_normal() : (parent / path).lexically_normal();
	}

	bool ReadNumber(const simdjson::dom::object& object, std::string_view key, float& value)
	{
		double parsed = 0.0;
		if (object.at_key(key).get(parsed)) return false;
		value = static_cast<float>(parsed);
		return std::isfinite(value);
	}

	bool ReadRectangle(const simdjson::dom::object& object, CommonUtilities::Rectanglef& rectangle)
	{
		return ReadNumber(object, "top", rectangle.Top) && ReadNumber(object, "left", rectangle.Left) &&
			ReadNumber(object, "bottom", rectangle.Bottom) && ReadNumber(object, "right", rectangle.Right);
	}
}

bool FontAsset::Load(const std::filesystem::path& path, AssetRegistry& registry)
{
	simdjson::dom::parser parser;
	simdjson::padded_string json;
	if (simdjson::padded_string::load(path.string()).get(json))
	{
		LOG(FontAssetLog, Warning, "Font metadata '{}' could not be read.", path.string());
		return false;
	}

	simdjson::dom::object root;
	if (parser.parse(json).get(root))
	{
		LOG(FontAssetLog, Warning, "Font metadata '{}' contains malformed JSON.", path.string());
		return false;
	}

	auto atlasResult = root.at_key("atlas");
	auto metricsResult = root.at_key("metrics");
	auto glyphsResult = root.at_key("glyphs");
	const std::string atlasFile = ReadString(root, "atlasFile");
	if (atlasResult.error() || !atlasResult.value().is_object() || metricsResult.error() || !metricsResult.value().is_object() ||
		glyphsResult.error() || !glyphsResult.value().is_array() || atlasFile.empty())
	{
		LOG(FontAssetLog, Warning, "Font metadata '{}' is missing atlas, metrics, glyphs, or atlasFile.", path.string());
		return false;
	}

	const auto atlas = atlasResult.value().get_object().value();
	const auto metrics = metricsResult.value().get_object().value();

	std::shared_ptr<Font> font = std::make_shared<Font>();
	float width = 0.0f, height = 0.0f;
	if (!ReadNumber(metrics, "lineHeight", font->myLineHeight) || !ReadNumber(metrics, "ascender", font->myAscender) ||
		!ReadNumber(metrics, "descender", font->myDescender) || !ReadNumber(atlas, "distanceRange", font->myDistanceRange) ||
		!ReadNumber(atlas, "width", width) || !ReadNumber(atlas, "height", height) || font->myLineHeight <= 0.0f ||
		width <= 0.0f || height <= 0.0f)
	{
		LOG(FontAssetLog, Warning, "Font metadata '{}' has missing or invalid line/atlas metrics.", path.string());
		return false;
	}

	font->myAtlasWidth = static_cast<unsigned>(width);
	font->myAtlasHeight = static_cast<unsigned>(height);
	myAtlasAsset = registry.GetAsset<TextureAsset>(ResolveRelative(path.parent_path(), atlasFile).generic_string());
	if (!myAtlasAsset)
	{
		const std::string& atlasError = registry.GetLastError();
		LOG(FontAssetLog, Warning, "Font metadata '{}' requires atlas '{}': {}", path.string(), atlasFile, atlasError);
		return false;
	}

	font->myAtlas = myAtlasAsset->GetTextureShared();
	for (const simdjson::dom::element element : glyphsResult.value().get_array().value())
	{
		if (!element.is_object())
		{
			continue;
		}
		const simdjson::dom::object glyphObject = element.get_object().value();
		uint64_t unicode = 0;
		Font::Glyph glyph;
		if (glyphObject.at_key("unicode").get(unicode) || !ReadNumber(glyphObject, "advance", glyph.Advance))
		{
			continue;
		}
		glyph.Unicode = static_cast<uint32_t>(unicode);
		auto planeResult = glyphObject.at_key("planeBounds");
		auto boundsResult = glyphObject.at_key("atlasBounds");
		if (!planeResult.error() && planeResult.value().is_object() && !boundsResult.error() && boundsResult.value().is_object())
		{
			glyph.HasGeometry = ReadRectangle(planeResult.value().get_object().value(), glyph.PlaneBounds) &&
				ReadRectangle(boundsResult.value().get_object().value(), glyph.AtlasBounds);
			if (!glyph.HasGeometry)
			{
				LOG(FontAssetLog, Warning, "Font metadata '{}' has malformed bounds for codepoint {}.", path.string(), unicode);
				return false;
			}
		}
		font->myGlyphs[glyph.Unicode] = glyph;
	}
	if (font->myGlyphs.empty() || !font->FindGlyph('?'))
	{
		LOG(FontAssetLog, Warning, "Font metadata '{}' contains no usable glyphs or '?' fallback.", path.string());
		return false;
	}

	myFont = font;
	return true;
}
