#include "GraphicsEngine.pch.h"
#include "GraphicsEngine.h"

#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/ObjectBuffer.h"
#include "GameFramework/CameraComponent.h"
#include "GameFramework/MeshComponent.h"
#include "GameFramework/World.h"

/*
 * GraphicsEngine handles rendering of a scene.
 * Handles culling using whatever cameras it's told to use.
 * Renders onto a user-provided render target and depth.
 */

// TODO: Temporary Shader Includes
#include "GraphicsEngine/TemporaryShaders/VertexShader.h"
#include "GraphicsEngine/TemporaryShaders/PixelShader.h"

GraphicsEngine& GraphicsEngine::Get()
{
	static GraphicsEngine myInstance;
	return myInstance;
}

bool GraphicsEngine::Initialize(HWND aWindowHandle)
{
	if (!myRHI.Initialize(aWindowHandle, true, myBackBuffer, myDepthBuffer))
	{
		return false; // RHI logs this for us 
	}
	
	CreateConstantBuffer<FrameBuffer>(ConstantBuffer::FrameBuffer, "FrameBuffer");
	CreateConstantBuffer<ObjectBuffer>(ConstantBuffer::ObjectBuffer, "ObjectBuffer");

	// TODO: Temporary PSO
	PipelineStateDescription tempPSODesc;
	tempPSODesc.Name = "TempPSO";
	tempPSODesc.VertexShader.ByteCode = TEMP_VertexShader_ByteCode;
	tempPSODesc.VertexShader.ByteCodeSize = sizeof(TEMP_VertexShader_ByteCode);
	tempPSODesc.PixelShader.ByteCode = TEMP_PixelShader_ByteCode;
	tempPSODesc.PixelShader.ByteCodeSize = sizeof(TEMP_PixelShader_ByteCode);
	tempPSODesc.InputLayoutElements = Vertex::Description;
	tempPSODesc.Topology = Topology::TriangleList;
	if (!myRHI.CreatePipelineStateObject(tempPSODesc, myTempPSO))
	{
		return false; // RHI logs this for us
	}
	// End Temporary PSO

	return true;
}

void GraphicsEngine::Render(const Actor& aCameraActor, const World& aWorld)
{
	myRHI.ClearRenderTarget(myBackBuffer);
	myRHI.ClearDepthStencil(myDepthBuffer);
	myRHI.SetRenderTarget(&myBackBuffer, &myDepthBuffer);
	myRHI.SetPipelineState(&myTempPSO); // TODO: Temporary PSO


	CameraComponent* cameraComponent = aCameraActor.GetComponent<CameraComponent>();
	if (cameraComponent == nullptr)
	{
		GELOG(Warning, "Could not render world because camera actor '{}' has no CameraComponent.", aCameraActor.GetName());
		myRHI.Present();
		return;
	}

	cameraComponent->SyncCameraToOwner();
	const CU::Camera3D& camera = cameraComponent->GetCamera();

	FrameBuffer fb;
	fb.View = camera.GetViewMatrix();
	fb.Projection = camera.GetProjectionMatrix();

	UpdateAndSetConstantBuffer(ConstantBuffer::FrameBuffer, fb, 0, PipeLineStage_VertexShader);

	for (const std::unique_ptr<Actor>& actor : aWorld.GetActors())
	{
		if (!actor || !actor->IsActive())
		{
			continue;
		}

		std::vector<MeshComponent*> meshComponents;
		actor->GetComponentsOfType(meshComponents);

		for (const MeshComponent* meshComponent : meshComponents)
		{
			if (meshComponent == nullptr || !meshComponent->IsEnabled() || !meshComponent->HasMesh())
			{
				continue;
			}

			RenderMesh(*meshComponent->GetMesh(), actor->GetTransform().GetWorldMatrix());
		}
	}

	myRHI.Present();
}

CU::Vector2u GraphicsEngine::GetClientSize() const
{
    return myRHI.GetClientSize();
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

void GraphicsEngine::RenderMesh(const Mesh& aMesh, const CU::Matrix4f& aWorld)
{
	if (!PrepareMeshForRendering(aMesh))
	{
		return;
	}

	myRHI.SetVertexBuffer(&aMesh.myVertexBuffer);
	myRHI.SetIndexBuffer(&aMesh.myIndexBuffer);

	ObjectBuffer ob;
	ob.World = aWorld;
	UpdateAndSetConstantBuffer(ConstantBuffer::ObjectBuffer, ob, 1, PipeLineStage_VertexShader);

	for (const Mesh::Element& element : aMesh.myElements)
	{
		myRHI.DrawIndexed(element.NumIndices, element.IndexOffset);
	}
}

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
