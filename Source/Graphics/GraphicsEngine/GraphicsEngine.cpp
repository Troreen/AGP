#include "GraphicsEngine.pch.h"
#include "GraphicsEngine.h"

#include "ConstantBuffers/AnimationBuffer.h"
#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/ObjectBuffer.h"
#include "GameFramework/CameraComponent.h"
#include "GameFramework/MeshComponentBase.h"
#include "GameFramework/World.h"
#include "RHI/GraphicsCommandList.h"
#include "RHI/RHIShaderReflectionInfo.h"

#include "Materials/Material.h"
#include "Materials/MaterialShaderIncludeHandler.h"
#include "Materials/MaterialHelpers.h"
#include "Objects/Shader.h"

GraphicsEngine& GraphicsEngine::Get()
{
	static GraphicsEngine myInstance;
	return myInstance;
}

bool GraphicsEngine::Initialize(HWND aWindowHandle, const std::filesystem::path& aShaderRoot)
{
	if (!std::filesystem::exists(aShaderRoot))
	{
		GELOG(Error, "Shader root dir is not a valid path! Provided root was {}.", aShaderRoot.string());
		return false;
	}

	myShaderRoot = aShaderRoot;

	if (!myRHI.Initialize(aWindowHandle, true, myBackBuffer, myDepthBuffer))
	{
		return false; // RHI logs this for us 
	}
	
	myMaterialDomainShaders.emplace(MaterialDomain::Surface, aShaderRoot / "Material" / "Surface_VS.hlsl");
	myMaterialShadingModelShaders.emplace(ShadingModel::Unlit, aShaderRoot / "Material" / "Unlit_PS.hlsl");

	MaterialDescription defaultMaterialDesc;
	defaultMaterialDesc.Name = "Default";
	defaultMaterialDesc.BlendMode = BlendMode::Opaque;
	defaultMaterialDesc.Domain = MaterialDomain::Surface;
	defaultMaterialDesc.ShadingModel = ShadingModel::Unlit;
	defaultMaterialDesc.MaterialShaderCode = aShaderRoot / "Material" / "Material.hlsli";
	if (!CreateMaterial(defaultMaterialDesc, myDefaultMaterial))
	{
		GELOG(Error, "Failed to create default material!");
		return false;
	}
	

	CreateConstantBuffer<FrameBuffer>(ConstantBuffer::FrameBuffer, "FrameBuffer");
	CreateConstantBuffer<ObjectBuffer>(ConstantBuffer::ObjectBuffer, "ObjectBuffer");
	CreateConstantBuffer<AnimationBuffer>(ConstantBuffer::AnimationBuffer, "AnimationBuffer");
	CreateConstantBuffer(ConstantBuffer::MaterialBuffer, "MaterialBuffer", Material::MATERIAL_BUFFER_SIZE);

	return true;
}

void GraphicsEngine::Render(GraphicsCommandList& inoutCommandList, const Actor& aCameraActor, const World& aWorld)
{
	inoutCommandList.ClearRenderTarget(myBackBuffer);
	inoutCommandList.ClearDepthStencil(myDepthBuffer);
	inoutCommandList.SetRenderTarget(&myBackBuffer, &myDepthBuffer);

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

		std::vector<MeshComponentBase*> meshComponents;
		actor->GetComponentsOfType(meshComponents);

		for (const MeshComponentBase* meshComponent : meshComponents)
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

bool GraphicsEngine::CreateConstantBuffer(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize)
{
	return CreateConstantBufferInternal(aBufferId, aName, aBufferSize);
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

bool GraphicsEngine::CreateMaterial(const MaterialDescription& aDescription, Material& outMaterial) const
{
	Shader materialVS;
	Shader materialPS;

	if (aDescription.ShadingModel == ShadingModel::None)
	{
		GELOG(Error, "Material {} has invalid shading model!", aDescription.Name);
		return false;
	}
	if (aDescription.Domain == MaterialDomain::None)
	{
		GELOG(Error, "Material {} has invalid material domain!", aDescription.Name);
		return false;
	}
	if(aDescription.BlendMode == BlendMode::None)
	{
		GELOG(Error, "Material {} has invalid blend mode!", aDescription.Name);
		return false;
	}
	if (aDescription.Name.empty())
	{
		GELOG(Error, "Material has no name!");
		return false;
	}
	
	{
		const std::filesystem::path& path = myMaterialDomainShaders.at(aDescription.Domain);
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::VertexShader, path, &handler, true, materialVS))
			return false;
	}

	{
		const std::filesystem::path& path = myMaterialShadingModelShaders.at(aDescription.ShadingModel);
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::PixelShader, path, &handler, true, materialPS))
			return false;
	}

	RHIShaderReflectionInfo vsInfo, psInfo;
	RHIShaderReflector::Reflect(materialVS.GetDataPtr(), materialVS.GetDataSize(), vsInfo);
	RHIShaderReflector::Reflect(materialPS.GetDataPtr(), materialPS.GetDataSize(), psInfo);
	
	const RHIShaderReflectionInfo* materialBufferSource = nullptr;
	static std::string materialBufferName = "MaterialBuffer";
	if (vsInfo.ConstantBufferNameToIndex.contains(materialBufferName))
	{
		materialBufferSource = &vsInfo;
	}
	else if (psInfo.ConstantBufferNameToIndex.contains(materialBufferName))
	{
		materialBufferSource = &psInfo;
	}
	
	memset(outMaterial.myData, 0, Material::MATERIAL_BUFFER_SIZE);

	if (materialBufferSource)
	{
		const RHIShaderReflectionInfo::ConstantBufferInfo& info = materialBufferSource->ConstantBuffers[materialBufferSource->ConstantBufferNameToIndex.at(materialBufferName)];
		
		//for (const auto& member : info.Members)
		for (size_t i = 0; i < info.Members.size(); ++i)
		{
			const auto& member = info.Members[i];
			MaterialParameterInfo param;
			param.Name = member.Name;
			param.Type = MaterialHelpers::HLSLTypeToMaterialParameterType(member.Type);
			param.Size = member.Size;
			param.Offset = member.Offset;

			memcpy_s(outMaterial.myData + param.Offset, param.Size, member.Default, param.Size);

			outMaterial.myParameterNameToIndex.emplace(param.Name, static_cast<unsigned>(outMaterial.myParameters.size()));
			outMaterial.myParameters.emplace_back(std::move(param));
		
		}
	}


	PipelineStateDescription matPSOdesc;
	matPSOdesc.Name = std::format("{}_MAT_PSO", aDescription.Name);
	matPSOdesc.VertexShader.ByteCode = materialVS.GetDataPtr();
	matPSOdesc.VertexShader.ByteCodeSize = materialVS.GetDataSize();
	matPSOdesc.PixelShader.ByteCode = materialPS.GetDataPtr();
	matPSOdesc.PixelShader.ByteCodeSize = materialPS.GetDataSize();
	matPSOdesc.InputLayoutElements = Vertex::Description;
	matPSOdesc.Topology = Topology::TriangleList;

	PipelineStateObject matPSO;
	if (!myRHI.CreatePipelineStateObject(matPSOdesc, matPSO))
	{
		return false;
	}

	outMaterial.myPSO = matPSO;
	outMaterial.myName = aDescription.Name;
	outMaterial.myDescription = aDescription;

	return true;
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

void GraphicsEngine::RenderMesh(GraphicsCommandList& inoutCommandList, const MeshComponentBase& aMeshComponent, const CU::Matrix4f& aWorld)
{
	const std::shared_ptr<Mesh>& mesh = aMeshComponent.GetMesh();
	const std::vector<std::shared_ptr<MaterialInterface>>& materials = aMeshComponent.GetMaterialList();
	if (mesh == nullptr)
	{
		return;
	}

	if (!PrepareMeshForRendering(*mesh))
	{
		return;
	}

	inoutCommandList.SetVertexBuffer(&mesh->myVertexBuffer);
	inoutCommandList.SetIndexBuffer(&mesh->myIndexBuffer);

	ObjectBuffer ob;
	ob.World = aWorld;
	ob.HasSkinning = aMeshComponent.HasSkinning() ? 1u : 0u;
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::ObjectBuffer, ob, 1, PipeLineStage_VertexShader);

	if (aMeshComponent.HasSkinning())
	{
		AnimationBuffer animationBuffer;
		const std::array<CU::Matrix4f, 128>* jointTransforms = aMeshComponent.GetJointTransforms();
		if (jointTransforms != nullptr)
		{
			animationBuffer.JointTransforms = *jointTransforms;
			UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::AnimationBuffer, animationBuffer, 2, PipeLineStage_VertexShader);
		}
	}

	MaterialInterface* currentMaterial = nullptr;
	for (const Mesh::Element& element : mesh->myElements)
	{
		MaterialInterface* elementMaterial = &myDefaultMaterial;
		if (element.MaterialIndex < materials.size() && materials[element.MaterialIndex] != nullptr)
		{
			elementMaterial = materials[element.MaterialIndex].get();
		}

		if (elementMaterial != currentMaterial)
		{
			currentMaterial = elementMaterial;
			inoutCommandList.SetPipelineState(&currentMaterial->GetPSO());
		}

		if (currentMaterial->HasParameters())
		{
			if (currentMaterial->IsMaterialDataDirty())
			{
				currentMaterial->RefreshMaterialData();
			}

			UpdateAndSetConstantBufferInternal(inoutCommandList, ConstantBuffer::MaterialBuffer, currentMaterial->GetParameterDataBlock(), 
				Material::MATERIAL_BUFFER_SIZE, 3, PipeLineStage_VertexShader | PipeLineStage_PixelShader);
		}
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
