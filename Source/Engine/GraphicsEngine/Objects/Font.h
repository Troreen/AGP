#pragma once

#include "Rectangle.h"
#include "GraphicsEngine/Objects/Texture.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

class AssetRegistry;

class Font
{
public:
	struct Glyph
	{
		uint32_t Unicode = 0;
		float Advance = 0.0f;
		CommonUtilities::Rectanglef PlaneBounds;
		CommonUtilities::Rectanglef AtlasBounds;
		bool HasGeometry = false;
	};

	const Glyph* FindGlyph(uint32_t codepoint) const
	{
		const auto found = myGlyphs.find(codepoint);
		return found == myGlyphs.end() ? nullptr : &found->second;
	}

	float GetLineHeight() const { return myLineHeight; }
	float GetAscender() const { return myAscender; }
	float GetDescender() const { return myDescender; }
	float GetDistanceRange() const { return myDistanceRange; }
	unsigned GetAtlasWidth() const { return myAtlasWidth; }
	unsigned GetAtlasHeight() const { return myAtlasHeight; }
	const std::shared_ptr<Texture>& GetAtlas() const { return myAtlas; }

private:
	float myLineHeight = 0.0f;
	float myAscender = 0.0f;
	float myDescender = 0.0f;
	float myDistanceRange = 0.0f;
	unsigned myAtlasWidth = 0;
	unsigned myAtlasHeight = 0;
	std::unordered_map<uint32_t, Glyph> myGlyphs;
	std::shared_ptr<Texture> myAtlas;

	friend class AssetRegistry;
	friend struct FontTestAccess;
};
