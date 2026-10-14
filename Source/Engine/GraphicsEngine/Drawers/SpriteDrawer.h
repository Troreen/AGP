#pragma once
#include "../Sprite/SpriteStructs.h"

class SpriteDrawer
{
	friend class GraphicsEngine;

public:
	static SpriteDrawer& Get();

	void AddDrawCall(const SpriteDrawCall& aDrawCall);
	std::vector<SpriteDrawCall> GetDrawCalls();
	void Clear();

private:
	SpriteDrawer() = default;

	std::vector<SpriteDrawCall> myDrawCalls;
};

