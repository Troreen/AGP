#pragma once

#include <array>

#include "Texture.h"

class GBuffer
{
public:
	static constexpr size_t TargetCount = 6;
	// Keep both normal spaces: lighting consumes PixelNormal (world space), while
	// the tangent-space value is retained for the render-pass debugger.
	enum Target : size_t { Albedo, PixelNormal, Material, VertexNormal, WorldPosition, TangentNormal };

	const std::array<Texture, TargetCount>& GetTextures() const { return myTextures; }
	std::array<Texture, TargetCount>& GetTextures() { return myTextures; }

private:
	std::array<Texture, TargetCount> myTextures;
};
