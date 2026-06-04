#include "GraphicsEngine.pch.h"
#include "GraphicsEngine.h"

#include "GraphicsEngine/Objects/Vertex.h"

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
	if (!myRHI.Initialize(aWindowHandle, true, myBackBuffer))
	{
		return false; // RHI logs this for us 
	}
	// TODO: Temporary code
	std::vector<Vertex> vertices(3);
	vertices[0] = { .Position = { 0, 0.75f, 0, 1 }, .Color = { 1, 0, 0, 1 } };
	vertices[1] = { .Position = { 0.75f, -0.75f, 0, 1 }, .Color = { 0, 1, 0, 1 } };
	vertices[2] = { .Position = { -0.75f, -0.75f, 0, 1 }, .Color = { 0, 0, 1, 1 } };

	myRHI.CreateVertexBuffer("Triangle", vertices, myTempBuffer);
	
	return true;
}

void GraphicsEngine::Render()
{
	myRHI.ClearRenderTarget(myBackBuffer);
	myRHI.SetRenderTarget(&myBackBuffer);

	myRHI.SetVertexBuffer(&myTempBuffer);
	myRHI.Draw(3);

	myRHI.Present();	
}

GraphicsEngine::GraphicsEngine() = default;
GraphicsEngine::~GraphicsEngine() = default;
