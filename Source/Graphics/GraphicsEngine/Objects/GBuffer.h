#pragma once

#include <array>

#include "Texture.h"

class GBuffer
{
public:
	static constexpr size_t TargetCount = 5;
	enum Target : size_t { Albedo, PixelNormal, Material, VertexNormal, WorldPosition };

	const std::array<Texture, TargetCount>& GetTextures() const { return myTextures; }
	std::array<Texture, TargetCount>& GetTextures() { return myTextures; }

private:
	std::array<Texture, TargetCount> myTextures;
};
