#include "GraphicsEngine.pch.h"
#include "GraphicsEngine.h"

#include "ConstantBuffers/AnimationBuffer.h"
#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/ObjectBuffer.h"
#include "GameFramework/CameraComponent.h"
#include "GameFramework/MeshComponent.h"
#include "GameFramework/World.h"
#include "RHI/GraphicsCommandList.h"
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
	CreateConstantBuffer<AnimationBuffer>(ConstantBuffer::AnimationBuffer, "AnimationBuffer");

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

void GraphicsEngine::Render(GraphicsCommandList& inoutCommandList, const Actor& aCameraActor, const World& aWorld)
{
	inoutCommandList.ClearRenderTarget(myBackBuffer);
	inoutCommandList.ClearDepthStencil(myDepthBuffer);
	inoutCommandList.SetRenderTarget(&myBackBuffer, &myDepthBuffer);
	inoutCommandList.SetPipelineState(&myTempPSO); // TODO: Temporary PSO


	CameraComponent* cameraComponent = aCameraActor.GetComponent<CameraComponent>();
	if (cameraComponent == nullptr)
	{
		GELOG(Warning, "Could not render world because camera actor '{}' has no CameraComponent.", aCameraActor.GetName());
		return;
	}

	cameraComponent->SyncCameraToOwner();
	const CU::Camera3D& camera = cameraComponent->GetCamera();

	FrameBuffer fb;
	fb.View = camera.GetViewMatrix();
	fb.Projection = camera.GetProjectionMatrix();

	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, fb, 0, PipeLineStage_VertexShader);

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

			RenderMesh(inoutCommandList, *meshComponent, actor->GetTransform().GetWorldMatrix());
		}
	}
}

void GraphicsEngine::Present() const
{
	myRHI.Present();
}

CU::Vector2u GraphicsEngine::GetClientSize() const
{
    return myRHI.GetClientSize();
}

bool GraphicsEngine::CreateCommandList(std::string_view aName, GraphicsCommandList &outCommandList) const
{
    return myRHI.CreateCommandList(aName, outCommandList);
}

void GraphicsEngine::ExecuteCommandList(const GraphicsCommandList &aCommandList) const
{
	myRHI.ExecuteCommandList(aCommandList);
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

bool GraphicsEngine::UpdateAndSetConstantBufferInternal(GraphicsCommandList& inoutCommandList, ConstantBuffer aBufferId, const void *aData, size_t aDataSize, unsigned aSlot, PipeLineStages aStages)
{
    if (!myConstantBuffers.contains(aBufferId))
	{
		GELOG(Warning, "Requested constant buffer update failed because this buffer does not exist!");
		return false;	
	}

	const Buffer& buffer = myConstantBuffers.at(aBufferId);
	if (!inoutCommandList.UpdateConstantBuffer(buffer, aData, aDataSize))
	{
		return false;
	}

	inoutCommandList.SetConstantBuffer(&buffer, aSlot, aStages);
	return true;
}

GraphicsEngine::GraphicsEngine() = default;
GraphicsEngine::~GraphicsEngine() = default;

void GraphicsEngine::RenderMesh(GraphicsCommandList& inoutCommandList, const MeshComponent& aMeshComponent, const CU::Matrix4f& aWorld)
{
	const std::shared_ptr<Mesh> mesh = aMeshComponent.GetMesh();
	if (mesh == nullptr)
	{
		return;
	}

	const Mesh& aMesh = *mesh;
	if (!PrepareMeshForRendering(aMesh))
	{
		return;
	}

	inoutCommandList.SetVertexBuffer(&aMesh.myVertexBuffer);
	inoutCommandList.SetIndexBuffer(&aMesh.myIndexBuffer);

	ObjectBuffer ob;
	ob.World = aWorld;
	ob.HasSkinning = aMeshComponent.HasSkinning() ? 1u : 0u;
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::ObjectBuffer, ob, 1, PipeLineStage_VertexShader);

	if (aMeshComponent.HasSkinning())
	{
		AnimationBuffer animationBuffer;
		animationBuffer.JointTransforms = aMeshComponent.GetJointTransforms();
		UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::AnimationBuffer, animationBuffer, 2, PipeLineStage_VertexShader);
	}

	for (const Mesh::Element& element : aMesh.myElements)
	{
		inoutCommandList.DrawIndexed(element.NumIndices, element.IndexOffset);
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
