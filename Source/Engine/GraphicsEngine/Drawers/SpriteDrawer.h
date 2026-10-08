#pragma once
#include "../Sprite/SpriteStructs.h"
#include <Matrix.hpp>
#include <GraphicsEngine/Materials/MaterialInterface.h>

struct SpriteDrawCall
{
	const SpriteInstanceData* SpriteInstanceData = nullptr;
	const SpriteSharedData* SpriteSharedData = nullptr;
	const MaterialInterface* Material;
};
class SpriteDrawer
{
	friend class GraphicsEngine;

public:

	static SpriteDrawer& Get()
	{
		static SpriteDrawer instance;
		return instance;
	}

	void AddDrawCall(const SpriteDrawCall& aDrawCall);

	std::vector<SpriteDrawCall> GetDrawCalls();

private:

	SpriteDrawer() = default;

	std::vector<SpriteDrawCall> myDrawCalls;

};

