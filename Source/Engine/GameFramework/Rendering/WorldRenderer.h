#pragma once
#include "GraphicsEngine/GraphicsEngine.h"
class World;

class WorldRenderer
{
public:
	static void Build(const World& world, GraphicsEngine& graphics, GraphicsEngine::RenderSceneSnapshot& snapshot);
};
