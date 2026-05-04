#include "GraphicsEngine.pch.h"
#include "GraphicsEngine.h"

/*
 * GraphicsEngine handles rendering of a scene.
 * Handles culling using whatever cameras it's told to use.
 * Renders onto a user-provided render target and depth.
 */

GraphicsEngine& GraphicsEngine::Get()
{
	static GraphicsEngine myInstance;
	return myInstance;
}

bool GraphicsEngine::Initialize(HWND aWindowHandle)
{
	return true;
}

void GraphicsEngine::Render()
{

}

GraphicsEngine::GraphicsEngine() = default;
GraphicsEngine::~GraphicsEngine() = default;