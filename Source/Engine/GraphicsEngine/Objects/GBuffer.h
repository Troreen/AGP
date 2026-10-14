#pragma once

#include "Texture.h"

#include <array>
#include <string>

class GBuffer
{
	friend class RenderHardwareInterface;
	friend class GraphicsCommandList;

public:
	GBuffer() = default;
	~GBuffer() = default;

	// Pointer to the first texture in myTextures.
	const Texture* GetTextures() const { return myTextures.data(); }
	const Texture** GetTexturePointers() { return myTexturePointers.data(); }
	// Number of textures available.
	static constexpr unsigned NumGBufferTargets = 5;

private:
	std::array<Texture, NumGBufferTargets> myTextures;
	std::array<const Texture*, NumGBufferTargets> myTexturePointers{};
	std::string myName;
};
