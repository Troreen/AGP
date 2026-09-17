#pragma once
#include "GraphicsEngine/GraphicsEngine.h"

class World;

namespace GameFrameworkInternal
{
	class WorldRenderBridge
	{
	public:
		// Always replaces the output. False is an empty presentation which the
		// host must publish, not permission to retain an earlier scene image.
		static bool Build(const World& world, GraphicsEngine& graphics, GraphicsEngine::RenderSceneSnapshot& snapshot);
	};
}
