#include "GraphicsEngine.pch.h"
#include "GraphicsEngine.h"

#include "ConstantBuffers/AnimationBuffer.h"
#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/LightBuffer.h"
#include "ConstantBuffers/ObjectBuffer.h"
#include "GameFramework/CameraComponent.h"
#include "GameFramework/LightComponent.h"
#include "GameFramework/MeshComponentBase.h"
#include "GameFramework/World.h"
#include "RHI/GraphicsCommandList.h"
#include "RHI/RHIShaderReflectionInfo.h"

#include "Materials/Material.h"
#include "Materials/MaterialShaderIncludeHandler.h"
#include "Materials/MaterialHelpers.h"
#include "Objects/Shader.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace
{
	float GetRenderIntensity(const LightComponent& aLightComponent)
	{
		if (aLightComponent.GetLightType() == LightType::Directional)
		{
			return aLightComponent.GetIntensity();
		}

		return aLightComponent.GetIntensity() * 10000.0f;
	}

	void AddLightToBuffer(const LightComponent& aLightComponent, LightBuffer& inoutLightBuffer)
	{
		if (inoutLightBuffer.NumActiveLights >= LightBuffer::MaxLights)
		{
			return;
		}

		LightBuffer::Light& light = inoutLightBuffer.Lights[inoutLightBuffer.NumActiveLights++];
		light.Color = aLightComponent.GetColor();
		light.Intensity = GetRenderIntensity(aLightComponent);
		light.Position = aLightComponent.GetWorldPosition();
		light.Type = static_cast<unsigned>(aLightComponent.GetLightType());
		light.Direction = aLightComponent.GetWorldDirection();
		light.InnerCone = aLightComponent.GetInnerCone();
		light.OuterCone = aLightComponent.GetOuterCone();
		light.Radius = aLightComponent.GetRadius();
	}
}

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

	if (!CreateDefaultTextures())
	{
		return false;
	}
	
	myMaterialDomainShaders.emplace(MaterialDomain::Surface, aShaderRoot / "Material" / "Surface_VS.hlsl");
	myMaterialShadingModelShaders.emplace(ShadingModel::Unlit, aShaderRoot / "Material" / "Unlit_PS.hlsl");
	myMaterialShadingModelShaders.emplace(ShadingModel::Lit, aShaderRoot / "Material" / "Lit_PS.hlsl");

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
	CreateConstantBuffer<LightBuffer>(ConstantBuffer::LightBuffer, "LightBuffer");

	{ // Trilinear Wrap
		SamplerDescription samplerDesc;
		samplerDesc.Name = "TrilinearWrap";
		samplerDesc.AddressMode = SamplerAddressMode::Wrap;
		samplerDesc.FilterMode = SamplerFilterMode::Trilinear;
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}

	return true;
}

void GraphicsEngine::Render(GraphicsCommandList& inoutCommandList, const Actor& aCameraActor, const World& aWorld)
{
	inoutCommandList.ClearRenderTarget(myBackBuffer);
	inoutCommandList.ClearDepthStencil(myDepthBuffer);
	inoutCommandList.SetRenderTarget(&myBackBuffer, &myDepthBuffer);

	std::vector<Sampler*> samplerList(mySamplers.size());
	for (size_t s = 0; s < mySamplers.size(); ++s)
	{
		samplerList[s] = &mySamplers[s];
	}

	inoutCommandList.SetShaderSamplers(samplerList.data(), samplerList.size(), 0, PipeLineStage_VertexShader | PipeLineStage_PixelShader);

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

	LightBuffer lightBuffer;
	for (const std::unique_ptr<Actor>& actor : aWorld.GetActors())
	{
		if (!actor || !actor->IsActive())
		{
			continue;
		}

		std::vector<LightComponent*> lightComponents;
		actor->GetComponentsOfType(lightComponents);
		for (const LightComponent* lightComponent : lightComponents)
		{
			if (lightComponent == nullptr || !lightComponent->IsEnabled())
			{
				continue;
			}

			AddLightToBuffer(*lightComponent, lightBuffer);
		}
	}
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, lightBuffer, 4, PipeLineStage_PixelShader);

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

	memset(outMaterial.myData, 0, Material::MATERIAL_BUFFER_SIZE);
	outMaterial.myParameters.clear();
	outMaterial.myParameterNameToIndex.clear();
	outMaterial.myTextureSlotNameToIndex.clear();
	for (std::shared_ptr<Texture>& texture : outMaterial.myTextures)
	{
		texture.reset();
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
	
	if (materialBufferSource)
	{
		const RHIShaderReflectionInfo::ConstantBufferInfo& info = materialBufferSource->ConstantBuffers[materialBufferSource->ConstantBufferNameToIndex.at(materialBufferName)];
		
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

	CreateMaterialTextureSlots(vsInfo, outMaterial);
	CreateMaterialTextureSlots(psInfo, outMaterial);

	auto loadTextureOrFallback = [this](const std::filesystem::path& aTexturePath, const std::shared_ptr<Texture>& aFallback, std::string_view aTextureLabel)
	{
		if (!aTexturePath.empty())
		{
			std::shared_ptr<Texture> texture = std::make_shared<Texture>();
			if (LoadTexture(aTexturePath, *texture))
			{
				return texture;
			}

			GELOG(Warning, "Falling back to default {} texture because {} could not be loaded.", aTextureLabel, aTexturePath.string());
		}

		return aFallback;
	};

	outMaterial.SetTexture(Material::ALBEDO_TEXTURE_SLOT, loadTextureOrFallback(aDescription.AlbedoTexture, myDefaultAlbedoTexture, "albedo"));
	outMaterial.SetTexture(Material::NORMAL_TEXTURE_SLOT, loadTextureOrFallback(aDescription.NormalTexture, myDefaultNormalTexture, "normal"));

	outMaterial.myPSO = matPSO;
	outMaterial.myName = aDescription.Name;
	outMaterial.myDescription = aDescription;

	return true;
}

bool GraphicsEngine::CreateDefaultTextures()
{
	myDefaultAlbedoTexture = std::make_shared<Texture>();
	if (!myRHI.CreateColorTexture("Default_Albedo_White", std::array<uint8_t, 4>{ 255, 255, 255, 255 }, *myDefaultAlbedoTexture))
	{
		GELOG(Error, "Failed to create default albedo texture.");
		return false;
	}

	myDefaultNormalTexture = std::make_shared<Texture>();
	if (!myRHI.CreateColorTexture("Default_Normal_Flat", std::array<uint8_t, 4>{ 128, 128, 255, 255 }, *myDefaultNormalTexture))
	{
		GELOG(Error, "Failed to create default normal texture.");
		return false;
	}

	return true;
}

bool GraphicsEngine::LoadTexture(const std::filesystem::path& aPath, Texture& outTexture) const
{
	if (!std::filesystem::exists(aPath))
	{
		GELOG(Warning, "Texture path {} does not exist!", aPath.string());
		return false;
	}

	std::ifstream file(aPath, std::ios::binary | std::ios::ate);
	if (!file)
	{
		GELOG(Error, "Failed to load texture {}! Could not open file!", aPath.string());
		return false;
	}

	const std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> fileData(size);
	file.read(reinterpret_cast<char*>(fileData.data()), size);
	file.close();

	return myRHI.CreateTexture(aPath.stem().string(), fileData.data(), fileData.size(), outTexture);
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

void GraphicsEngine::CreateMaterialTextureSlots(const RHIShaderReflectionInfo& aShaderInfo, Material& inoutMaterial) const
{
	for (const auto& shaderTextureSlot : aShaderInfo.Bindings)
	{
		if (shaderTextureSlot.Type != 2 || shaderTextureSlot.BindPoint >= Material::MAX_MATERIAL_TEXTURE_COUNT)
			continue;

		std::string lowerName = shaderTextureSlot.Name;
		std::ranges::transform(lowerName, lowerName.begin(), [](unsigned char aChar)
		{
			return static_cast<char>(std::tolower(aChar));
		});

		if (inoutMaterial.myTextureSlotNameToIndex.contains(lowerName) && inoutMaterial.myTextureSlotNameToIndex.at(lowerName) != shaderTextureSlot.BindPoint)
		{
			GELOG(Warning, "Found texture {} in multiple places when setting up material. Only the first instance will be used!", shaderTextureSlot.Name);
			continue;
		}

		inoutMaterial.myTextureSlotNameToIndex.emplace(lowerName, shaderTextureSlot.BindPoint);
	}

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
	ob.WorldInvT = aWorld.GetInverseTranspose3x3();
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
		std::vector<const Texture*> textures(Material::MAX_MATERIAL_TEXTURE_COUNT);
		for (size_t t = 0; t < Material::MAX_MATERIAL_TEXTURE_COUNT; ++t)
		{
			if (const std::shared_ptr<Texture>& texture = currentMaterial->GetTexture(static_cast<unsigned>(t)))
			{
				textures[t] = texture.get();
			}
		}
		inoutCommandList.SetShaderResources(textures.data(), textures.size(), 0, PipeLineStage_VertexShader | PipeLineStage_PixelShader);

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
