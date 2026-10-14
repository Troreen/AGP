#pragma once

#include <Vector.hpp>

class MaterialInterface;
class Texture;

struct SpriteSharedData
{
	std::shared_ptr<Texture> Texture;
};

struct SpriteInstanceData
{
	CommonUtilities::Vector2f Size = { 1.0f, 1.0f };
	CommonUtilities::Vector2f Pivot = { 0.5f, 0.5f };
	CommonUtilities::Vector2f Anchor = { 0.0f, 0.0f };
	CommonUtilities::Vector2f Position = { 0.0f, 0.0f };

	CommonUtilities::Vector4f Color = { 1, 1, 1, 1 };

	float Depth = 0;

	bool IsVisible = true;
};

struct SpriteDrawCall
{
	const SpriteInstanceData* SpriteInstanceData = nullptr;
	const SpriteSharedData* SpriteSharedData = nullptr;
	const MaterialInterface* Material;
};
