#pragma once

#include <array>

#include "Texture.h"

class GBuffer
{
public:
	static constexpr size_t TargetCount = 5;

	// The compact deferred layout intentionally stores only the calculated pixel
	// normal. Interpolated vertex normals are not needed by deferred lighting.
	enum Target : size_t
	{
		Albedo,
		PixelNormal,
		Surface,
		Emission,
		WorldPosition
	};

	const std::array<Texture, TargetCount>& GetTextures() const
	{
		return myTextures;
	}

	std::array<Texture, TargetCount>& GetTextures()
	{
		return myTextures;
	}

private:
	std::array<Texture, TargetCount> myTextures;
};
