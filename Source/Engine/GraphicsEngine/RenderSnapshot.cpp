#include "GraphicsEngine.pch.h"
#include "RenderSnapshot.h"

void RenderSnapshot::Clear()
{
	ActiveCamera = nullptr;
	Meshes.clear();
	Lights.clear();
	ScreenTextItems.clear();
}
