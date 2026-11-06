#include "GraphicsEngine.pch.h"
#include "RenderSnapshot.h"

void RenderSnapshot::Clear()
{
	TotalTime = 0.0f;
	ActiveCamera = nullptr;
	AllMeshes.clear();
	OpaqueMeshes.clear();
	BlendedMeshes.clear();
	Lights.clear();
	Particles.clear();
	ScreenTextItems.clear();
}
