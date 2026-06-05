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
	
	return true;
}

void GraphicsEngine::Render(const Mesh& aMesh)
{
	myRHI.ClearRenderTarget(myBackBuffer);
	myRHI.SetRenderTarget(&myBackBuffer);

	if (PrepareMeshForRendering(aMesh))
	{
		myRHI.SetVertexBuffer(&aMesh.myVertexBuffer);		
		myRHI.SetIndexBuffer(&aMesh.myIndexBuffer);

		for	(const Mesh::Element& element : aMesh.myElements)
		{
			myRHI.DrawIndexed(element.NumIndices, element.IndexOffset);
		}
	}

	myRHI.Present();	
}

bool GraphicsEngine::CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize)
{
	Buffer buffer;
	if (!myRHI.CreateConstantBuffer(aName, aBufferSize, buffer))
	{
		return false;
	}

	myConstantBuffers.emplace(aBufferId, std::move(buffer));
	return true;
}

bool GraphicsEngine::UpdateAndSetConstantBufferInternal(ConstantBuffer aBufferId, const void *aData, size_t aDataSize, unsigned aSlot, PipeLineStages aStages)
{
    if (!myConstantBuffers.contains(aBufferId))
	{
		GELOG(Warning, "Requested constant buffer update failed because this buffer does not exist!");
		return false;	
	}

	const Buffer& buffer = myConstantBuffers.at(aBufferId);
	if (!myRHI.UpdateConstantBuffer(buffer, aData, aDataSize))
	{
		return false;
	}

	myRHI.SetConstantBuffer(&buffer, aSlot, aStages);
	return true;
}

GraphicsEngine::GraphicsEngine() = default;
GraphicsEngine::~GraphicsEngine() = default;

bool GraphicsEngine::PrepareMeshForRendering(const Mesh &aMesh) const
{
    if (!aMesh.myVertexBuffer.IsValid())
	{
		if (!myRHI.CreateVertexBuffer(aMesh.myName, aMesh.myVertices, aMesh.myVertexBuffer))
		{
			GELOG(Warning, "Could not prepare mesh {}! Vertex buffer could not be created!", aMesh.myName);
			return false;
		}
	}

	if (!aMesh.myIndexBuffer.IsValid())
	{
		if (!myRHI.CreateIndexBuffer(aMesh.myName, aMesh.myIndices, aMesh.myIndexBuffer))
		{
			GELOG(Warning, "Could not prepare mesh {}! Index buffer could not be created!", aMesh.myName);
			return false;
		}
	}

	return true;
}
