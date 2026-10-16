#include "GraphicsEngine.pch.h"
#include "RenderSnapshot.h"

void RenderSnapshot::Clear()
{
	TotalTime = 0.0f;
	ActiveCamera = nullptr;
	Meshes.clear();
	Lights.clear();
	ScreenTextItems.clear();
}
