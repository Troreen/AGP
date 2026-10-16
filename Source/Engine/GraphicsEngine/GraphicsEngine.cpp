#include "GraphicsEngine.pch.h"
#include "ConstantBuffers/AnimationBuffer.h"
#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/ObjectBuffer.h"
#include "ConstantBuffers/TextOverlayBuffer.h"
#include "GraphicsEngine.h"
#include "Materials/Material.h"
#include "Materials/MaterialHelpers.h"
#include "Materials/MaterialShaderIncludeHandler.h"
#include "Objects/Shader.h"
#include "Objects/Vertex.h"
#include "RHI/GraphicsCommandList.h"
#include "RHI/RHIShaderReflectionInfo.h"

#include "Deferred_AmbientLight_PS.h"
#include "Deferred_DirLight_PS.h"
#include "Deferred_PointLight_PS.h"
#include "Deferred_SpotLight_PS.h"
#include "FullTexture_VS.h"
#include "ShadowCube_GS.h"
#include "Sprite_PS.h"
#include "Sprite_VS.h"
#include "TextOverlay_PS.h"
#include "TextOverlay_VS.h"
#include "Tonemapper_PS.h"
#include "Lines_PS.h"
#include "Lines_VS.h"

#include <GameFramework/AssetHandling/MaterialAsset.h>
#include <GameFramework/AssetHandling/MeshAsset.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Components/MeshComponentBase.h>
#include <GameFramework/World/Actor.h>
#include "Drawers/LineDrawer.h"
#include <Maths.hpp>

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

bool GraphicsEngine::Initialize(HWND aWindowHandle, const std::filesystem::path& aShaderRoot)
{
	if (!std::filesystem::exists(aShaderRoot))
	{
		GELOG(Error, "Shader root dir is not a valid path! Provided root was {}.", aShaderRoot.string());
		return false;
	}

	myShaderRoot = aShaderRoot;

	if (!myRHI.Initialize(aWindowHandle, true, myBackBuffer, myHDRBuffer, myDepthBuffer, myGBuffer))
	{
		GELOG(Error, "Failed to initialize RHI");
		return false;
	}

	myMaterialDomainShaders.emplace(MaterialDomain::Surface, aShaderRoot / "Material" / "Surface_VS.hlsl");
	myMaterialShadingModelShaders.emplace(ShadingModel::Unlit, aShaderRoot / "Material" / "Unlit_PS.hlsl");
	myMaterialShadingModelShaders.emplace(ShadingModel::Lit, aShaderRoot / "Material" / "Lit_PS.hlsl");

	MaterialDescription defaultMatDesc;
	defaultMatDesc.Name = "Default";
	defaultMatDesc.BlendMode = BlendMode::Opaque;
	defaultMatDesc.Domain = MaterialDomain::Surface;
	defaultMatDesc.ShadingModel = ShadingModel::Unlit;
	defaultMatDesc.MaterialShaderCode = aShaderRoot / "Material" / "Material.hlsli";
	if (!CreateMaterial(defaultMatDesc, myDefaultMaterial))
	{
		GELOG(Error, "Failed to create default material!");
		return false;
	}

	if (!CreatePSOs())
	{
		GELOG(Error, "Failed to create PSOs");
		return false;
	}

	CreateConstantBuffer<FrameBuffer>(ConstantBuffer::FrameBuffer, "FrameBuffer");
	CreateConstantBuffer<LightBuffer>(ConstantBuffer::LightBuffer, "LightBuffer");
	CreateConstantBuffer<ObjectBuffer>(ConstantBuffer::ObjectBuffer, "ObjectBuffer");
	CreateConstantBuffer<AnimationBuffer>(ConstantBuffer::AnimationBuffer, "AnimationBuffer");
	CreateConstantBuffer(ConstantBuffer::MaterialBuffer, "MaterialBuffer", Material::MATERIAL_BUFFER_SIZE);
	CreateConstantBuffer(ConstantBuffer::PostProcessBuffer, "PostProcessBuffer", PostProcessEffect::POST_PROCESS_BUFFER_SIZE);
	CreateConstantBuffer<TextOverlayBuffer>(ConstantBuffer::TextOverlayBuffer, "TextOverlayBuffer");

	CreateSamplers();
	if (!GenerateLUT())
	{
		GELOG(Error, "Failed to generate a proper LUT!");
		return false;
	}

	myPostProcessCache.Initialize(myRHI);
	for (size_t i = 0; i < myPostProcessEffects.size(); ++i)
	{
		if (!myPostProcessEffects[i]->Initialize(myRHI))
		{
			return false;
		}
	}

	myPlainWhiteTexture = std::make_shared<Texture>();
	myFlatNormalTexture = std::make_shared<Texture>();
	myDefaultORMTexture = std::make_shared<Texture>();
	myNoEmissionTexture = std::make_shared<Texture>();
	myRHI.CreateColorTexture("Plain White", { 255, 255, 255, 255 }, *myPlainWhiteTexture);
	myRHI.CreateColorTexture("Flat Normal", { 128, 128, 255, 255 }, *myFlatNormalTexture);
	myRHI.CreateColorTexture("Default ORM", { 255, 128, 0  , 255 }, *myDefaultORMTexture);
	myRHI.CreateColorTexture("No Emission", { 0  , 0  , 0  , 0   }, *myNoEmissionTexture);

	myDefaultMaterial.SetTexture(0, myPlainWhiteTexture);
	myDefaultMaterial.SetTexture(1, myFlatNormalTexture);
	myDefaultMaterial.SetTexture(2, myDefaultORMTexture);
	myDefaultMaterial.SetTexture(3, myNoEmissionTexture);


	if (!myRHI.CreateColorCubeTexture("Default Environment", { 255, 255, 255, 255 }, myEnvironmentCubeTexture))
	{
		GELOG(Error, "Failed to create default enviroment Cube!");
		return false;
	}

	if (!myRHI.CreateDynamicVertexBuffer("SpriteBuffer", 6, mySpriteVertexBuffer))
	{
		GELOG(Error, "Failed to create sprite buffer!");
		return false;
	}

	if (!myRHI.CreateDynamicVertexBufferSimple("LineBuffer", 2048, myLineBuffer))
	{
		GELOG(Error, "Failed to create line buffer!");
		return false;
	}

	return true;
}

void GraphicsEngine::Render(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot, const GraphicsRenderSettings& aSettings)
{
	if (aSnapshot.ActiveCamera == nullptr)
	{
		return;
	}

	ResetPipeline(inoutCommandList);
	inoutCommandList.BeginEvent("Render Setup");

	std::vector<const Texture*> pblTextures = { &myEnvironmentCubeTexture, &myLut };
	inoutCommandList.SetShaderResources(pblTextures.data(), pblTextures.size(), 98, PipelineStage_PixelShader);

	FrameBuffer fb;
	fb.View = aSnapshot.ActiveCamera->GetView();
	fb.Projection = aSnapshot.ActiveCamera->GetProjection();
	fb.CameraPosition = aSnapshot.ActiveCamera->GetOwner()->GetTransform().GetWorldPosition();
	fb.TonemapType = static_cast<uint32_t>(aSettings.TonemapAlgorithm);
	fb.RenderPassType = static_cast<uint32_t>(aSettings.RenderPass);
	fb.TotalTime = aSnapshot.TotalTime;

	std::vector<MeshComponentBase*> opaqueMeshes(0);
	std::vector<MeshComponentBase*> blendedMeshes(0);
	SortMeshObjects(aSnapshot, fb.CameraPosition, opaqueMeshes, blendedMeshes);

	std::vector<Light> lights = SetupLights(inoutCommandList, aSnapshot);
	const Texture** gBuffer = CreateDeferredTextures(inoutCommandList, fb, opaqueMeshes);

	inoutCommandList.EndEvent(); // Render Setup

	inoutCommandList.SetRenderTarget(&myHDRBuffer, nullptr);

	RunPreLightPostProcess(inoutCommandList, gBuffer, aSettings);

	inoutCommandList.BeginEvent("Finalize Render");
	LightBuffer lb;
	lb.CascadeSplits.x = aSnapshot.ActiveCamera->GetCascadeSplit(0);
	lb.CascadeSplits.y = aSnapshot.ActiveCamera->GetCascadeSplit(1);
	lb.CascadeSplits.z = aSnapshot.ActiveCamera->GetCascadeSplit(2);
	RenderDeferredLight(inoutCommandList, aSettings, gBuffer, lb, lights);
	RenderBlendedObjects(inoutCommandList, blendedMeshes, lb, lights);
	inoutCommandList.EndEvent(); // Finalize Render

	RunPostLightPostProcess(inoutCommandList, aSettings);
	RunTonemap(inoutCommandList);
	RenderScreenText(inoutCommandList, aSnapshot);
	RenderScreenSprites(inoutCommandList, aSnapshot);
	RenderLines(inoutCommandList, fb);
}

void GraphicsEngine::Present() const
{
	myRHI.Present();
}

bool GraphicsEngine::SetEnvironmentMap(const std::filesystem::path& aCubeTexturePath)
{
	if (!LoadTexture(aCubeTexturePath, myEnvironmentCubeTexture))
	{
		GELOG(Error, "Failed to load environment cube map {}!", aCubeTexturePath.string());
		return false;
	}

	return true;
}

void GraphicsEngine::AddPostProcessEffect(const std::shared_ptr<PostProcessEffect>& aEffect)
{
	if (std::find(myPostProcessEffects.begin(), myPostProcessEffects.end(), aEffect) == myPostProcessEffects.end())
	{
		myPostProcessEffects.emplace_back(aEffect);
	}
}

void GraphicsEngine::RemovePostProcessEffect(const std::shared_ptr<PostProcessEffect>& aEffect)
{
	auto it = std::find(myPostProcessEffects.begin(), myPostProcessEffects.end(), aEffect);
	if (it != myPostProcessEffects.end())
	{
		myPostProcessEffects.erase(it);
	}
}

bool GraphicsEngine::CreateConstantBuffer(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize)
{
	return CreateConstantBufferInternal(aBufferId, aName, aBufferSize);
}

CU::Vector2u GraphicsEngine::GetClientSize() const
{
	return myRHI.GetClientSize();
}

bool GraphicsEngine::Resize(unsigned aWidth, unsigned aHeight)
{
	return myRHI.Resize(aWidth, aHeight, myBackBuffer, myDepthBuffer);
}

bool GraphicsEngine::CreateCommandList(std::string_view aName, GraphicsCommandList& outCommandList) const
{
	return myRHI.CreateCommandList(aName, outCommandList);
}

void GraphicsEngine::ExecuteCommandList(const GraphicsCommandList& aCommandList) const
{
	myRHI.ExecuteCommandList(aCommandList);
}

bool GraphicsEngine::CreateMaterial(const MaterialDescription& aDescription, Material& outMaterial) const
{
	Shader materialVS;
	Shader materialPS;

	if (aDescription.ShadingModel == ShadingModel::None || aDescription.Domain == MaterialDomain::None || aDescription.Name.empty())
	{
		GELOG(Error, "Material {} has invalid settings!", aDescription.Name);
		return false;
	}

	{
		const std::filesystem::path& path = myMaterialDomainShaders.at(aDescription.Domain);
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::VertexShader, path, &handler, true, materialVS))
		{
			return false;
		}
	}

	{
		const std::filesystem::path& path = aDescription.BlendMode != BlendMode::Opaque ?
			myMaterialShadingModelShaders.at(aDescription.ShadingModel) : (myShaderRoot / "Material" / "GBuffer_PS.hlsl");
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::PixelShader, path, &handler, true, materialPS))
		{
			return false;
		}
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

		for (size_t i = 0; i < info.Members.size(); ++i)
		{
			const auto& member = info.Members[i];
			MaterialParameterInfo param;
			param.Name = member.Name;
			param.Type = MaterialHelpers::HLSLTypeToMaterialParameterType(member.Type);
			param.Size = member.Size;
			param.Offset = member.Offset;
			param.Index = i;

			memcpy_s(outMaterial.myData + param.Offset, param.Size, member.Default, member.Size);

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
	matPSOdesc.FillMode = RasterizerState::FillSolid;
	matPSOdesc.CullMode = aDescription.CullMode;
	matPSOdesc.BlendMode = aDescription.BlendMode;

	PipelineStateObject matPSO;
	if (!myRHI.CreatePipelineStateObject(matPSOdesc, matPSO))
	{
		return false;
	}

	CreateMaterialTextureSlots(vsInfo, outMaterial);
	CreateMaterialTextureSlots(psInfo, outMaterial);

	outMaterial.SetTexture(0, myPlainWhiteTexture);
	outMaterial.SetTexture(1, myFlatNormalTexture);
	outMaterial.SetTexture(2, myDefaultORMTexture);
	outMaterial.SetTexture(3, myNoEmissionTexture);

	outMaterial.myPSO = matPSO;
	outMaterial.myName = aDescription.Name;
	outMaterial.myDescription = aDescription;
	GELOG(Verbose, "Material compiled: material='{}', shader='{}'.", aDescription.Name, aDescription.MaterialShaderCode.string());

	return true;
}

bool GraphicsEngine::LoadTexture(const std::filesystem::path& aPath, Texture& outTexture) const
{
	if (!std::filesystem::exists(aPath))
	{
		GELOG(Error, "Failed to load texture {}! File not found!", aPath.string());
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

bool GraphicsEngine::CreateTexture(std::string_view aName, unsigned aWidth, unsigned aHeight, unsigned aNumMips, ResourceFormat aFormat, ResourceBindFlags aBindFlags, Texture& outTexture) const
{
	return myRHI.CreateTexture(aName, aWidth, aHeight, aNumMips, aFormat, aBindFlags, outTexture);
}

bool GraphicsEngine::CreateShadowMap(std::string_view aName, unsigned aWidth, unsigned aHeight, Texture& outShadowMap, bool aCube) const
{
	return myRHI.CreateDepthStencil(aName, aWidth, aHeight, outShadowMap, aCube);
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

bool GraphicsEngine::UpdateAndSetConstantBufferInternal(GraphicsCommandList& inputCommandList, ConstantBuffer aBufferId, const void* aData, size_t aDataSize, unsigned aSlot, PipelineStages aStages)
{
	if (!myConstantBuffers.contains(aBufferId))
	{
		GELOG(Warning, "Requested constant buffer update failed because this buffer does not exist!");
		return false;
	}

	const Buffer& buffer = myConstantBuffers.at(aBufferId);
	if (!inputCommandList.UpdateConstantBuffer(buffer, aData, aDataSize))
	{
		return false;
	}

	inputCommandList.SetConstantBuffer(&buffer, aSlot, aStages);
	return true;
}

void GraphicsEngine::CreateMaterialTextureSlots(const RHIShaderReflectionInfo& aShaderInfo, Material& inoutMaterial) const
{
	for (const auto& shaderTextureSlot : aShaderInfo.Bindings)
	{
		if (shaderTextureSlot.Type != 2 || shaderTextureSlot.BindPoint >= Material::MAX_MATERIAL_TEXTURE_COUNT)
		{
			continue;
		}

		std::string lowerName = shaderTextureSlot.Name;
		std::ranges::transform(lowerName, lowerName.begin(), tolower);

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

bool GraphicsEngine::CreatePSOs()
{
	{
		PipelineStateDescription defferedAmbientLightDesc;
		defferedAmbientLightDesc.Name = "DefferedAmbientLightPSO";
		defferedAmbientLightDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		defferedAmbientLightDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		defferedAmbientLightDesc.PixelShader.ByteCode = INTERNAL_Deferred_AmbientLight_PS_ByteCode;
		defferedAmbientLightDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Deferred_AmbientLight_PS_ByteCode);
		defferedAmbientLightDesc.BlendMode = BlendMode::Additive;
		defferedAmbientLightDesc.Topology = Topology::TriangleStrip;

		myAmbientLightPSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(defferedAmbientLightDesc, *myAmbientLightPSO))
		{
			GELOG(Error, "Failed to create ambient light PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription defferedDirLightDesc;
		defferedDirLightDesc.Name = "DefferedDirLightPSO";
		defferedDirLightDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		defferedDirLightDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		defferedDirLightDesc.PixelShader.ByteCode = INTERNAL_Deferred_DirLight_PS_ByteCode;
		defferedDirLightDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Deferred_DirLight_PS_ByteCode);
		defferedDirLightDesc.BlendMode = BlendMode::Additive;
		defferedDirLightDesc.Topology = Topology::TriangleStrip;

		myDirLightPSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(defferedDirLightDesc, *myDirLightPSO))
		{
			GELOG(Error, "Failed to create directional light PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription defferedPointLightDesc;
		defferedPointLightDesc.Name = "DefferedPointLightPSO";
		defferedPointLightDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		defferedPointLightDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		defferedPointLightDesc.PixelShader.ByteCode = INTERNAL_Deferred_PointLight_PS_ByteCode;
		defferedPointLightDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Deferred_PointLight_PS_ByteCode);
		defferedPointLightDesc.BlendMode = BlendMode::Additive;
		defferedPointLightDesc.Topology = Topology::TriangleStrip;

		myPointLightPSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(defferedPointLightDesc, *myPointLightPSO))
		{
			GELOG(Error, "Failed to create point light PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription defferedSpotLightDesc;
		defferedSpotLightDesc.Name = "DefferedSpotLightPSO";
		defferedSpotLightDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		defferedSpotLightDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		defferedSpotLightDesc.PixelShader.ByteCode = INTERNAL_Deferred_SpotLight_PS_ByteCode;
		defferedSpotLightDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Deferred_SpotLight_PS_ByteCode);
		defferedSpotLightDesc.BlendMode = BlendMode::Additive;
		defferedSpotLightDesc.Topology = Topology::TriangleStrip;

		mySpotLightPSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(defferedSpotLightDesc, *mySpotLightPSO))
		{
			GELOG(Error, "Failed to create spot light PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription shadowPSODesc = {};
		shadowPSODesc.Name = "ShadowPSO";
		shadowPSODesc.FillMode = RasterizerState::FillSolid;
		shadowPSODesc.CullMode = RasterizerCullMode::Back;
		shadowPSODesc.GeometryShader.ByteCode = INTERNAL_ShadowCube_GS_ByteCode;
		shadowPSODesc.GeometryShader.ByteCodeSize = sizeof(INTERNAL_ShadowCube_GS_ByteCode);

		myShadowPSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(shadowPSODesc, *myShadowPSO))
		{
			GELOG(Error, "Failed to create shadow PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription tonemapDesc;
		tonemapDesc.Name = "TonemapPSO";
		tonemapDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		tonemapDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		tonemapDesc.PixelShader.ByteCode = INTERNAL_Tonemapper_PS_ByteCode;
		tonemapDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Tonemapper_PS_ByteCode);
		tonemapDesc.BlendMode = BlendMode::Opaque;
		tonemapDesc.Topology = Topology::TriangleStrip;

		myTonemapPSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(tonemapDesc, *myTonemapPSO))
		{
			GELOG(Error, "Failed to create tonemapper PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription textOverlayDesc;
		textOverlayDesc.Name = "TextOverlayPSO";
		textOverlayDesc.VertexShader.ByteCode = INTERNAL_TextOverlay_VS_ByteCode;
		textOverlayDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_TextOverlay_VS_ByteCode);
		textOverlayDesc.PixelShader.ByteCode = INTERNAL_TextOverlay_PS_ByteCode;
		textOverlayDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_TextOverlay_PS_ByteCode);
		textOverlayDesc.InputLayoutElements = Vertex::Description;
		textOverlayDesc.BlendMode = BlendMode::Alpha;
		textOverlayDesc.Topology = Topology::TriangleList;
		textOverlayDesc.CullMode = RasterizerCullMode::None;

		myTextOverlayPSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(textOverlayDesc, *myTextOverlayPSO))
		{
			GELOG(Error, "Failed to create text overlay PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription spriteDesc;
		spriteDesc.Name = "SpritePSO";
		spriteDesc.VertexShader.ByteCode = INTERNAL_Sprite_VS_ByteCode;
		spriteDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_Sprite_VS_ByteCode);
		spriteDesc.PixelShader.ByteCode = INTERNAL_Sprite_PS_ByteCode;
		spriteDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Sprite_PS_ByteCode);
		spriteDesc.InputLayoutElements = Vertex::Description;
		spriteDesc.Topology = Topology::TriangleList;
		spriteDesc.BlendMode = BlendMode::Opaque;
		spriteDesc.CullMode = RasterizerCullMode::None;

		mySpritePSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(spriteDesc, *mySpritePSO))
		{
			GELOG(Error, "Failed to create sprite PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription pipelineDesc;

		pipelineDesc.Name = "LinePSO";
		pipelineDesc.Topology = Topology::LineList;
		pipelineDesc.VertexShader.ByteCode = INTERNAL_Lines_VS_ByteCode;
		pipelineDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_Lines_VS_ByteCode);
		pipelineDesc.PixelShader.ByteCode = INTERNAL_Lines_PS_ByteCode;
		pipelineDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Lines_PS_ByteCode);
		pipelineDesc.InputLayoutElements = SimpleVertex::Description;

		myLinePSO = std::make_unique<PipelineStateObject>();
		if (!myRHI.CreatePipelineStateObject(pipelineDesc, *myLinePSO))
		{
			GELOG(Error, "Failed to create line PSO!");
			return false;
		}
	}

	return true;
}

void GraphicsEngine::CreateSamplers()
{
	{ // Shadow Sampler
		SamplerDescription samplerDesc;
		samplerDesc.Name = "ShadowSampler";
		samplerDesc.AddressMode = SamplerAddressMode::Border;
		samplerDesc.FilterMode = SamplerFilterMode::ComparisonLinearPoint;
		samplerDesc.ComparisonFunction = SamplerComparisonFunc::LessEqual;
		samplerDesc.BorderColor = CU::Vector4f(1, 1, 1, 1);
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}

	{ // Linear Clamp
		SamplerDescription samplerDesc;
		samplerDesc.Name = "LinearClamp";
		samplerDesc.AddressMode = SamplerAddressMode::Clamp;
		samplerDesc.FilterMode = SamplerFilterMode::Linear;
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}

	{ // Trilinear wrap
		SamplerDescription samplerDesc;
		samplerDesc.Name = "TrilinearWrap";
		samplerDesc.AddressMode = SamplerAddressMode::Wrap;
		samplerDesc.FilterMode = SamplerFilterMode::Trilinear;
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}

	{ // Trilinear Clamp
		SamplerDescription samplerDesc;
		samplerDesc.Name = "TrilinearClamp";
		samplerDesc.AddressMode = SamplerAddressMode::Clamp;
		samplerDesc.FilterMode = SamplerFilterMode::Trilinear;
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}
}

bool GraphicsEngine::GenerateLUT()
{
	if (!CreateTexture("LUT", 512, 512, 1, ResourceFormat::R16G16_FLOAT, ResourceBindFlag_RenderTarget | ResourceBindFlag_ShaderResource, myLut))
	{
		GELOG(Error, "Failed to create look up texture!");
		return false;
	}

	Shader lutVS;
	Shader lutPS;
	{
		const std::filesystem::path& path = myShaderRoot / "FullTexture_VS.hlsl";
		MaterialShaderIncludeHandler handler(myShaderRoot, path, "");
		if (!myRHI.CompileShader(ShaderType::VertexShader, path, &handler, true, lutVS))
		{
			return false;
		}
	}

	{
		const std::filesystem::path& path = myShaderRoot / "BRDF LUT_PS.hlsl";
		MaterialShaderIncludeHandler handler(myShaderRoot, path, "");
		if (!myRHI.CompileShader(ShaderType::PixelShader, path, &handler, true, lutPS))
		{
			return false;
		}
	}


	PipelineStateDescription lutPSODesc = {};
	lutPSODesc.Name = "LUT_PSO";
	lutPSODesc.Topology = Topology::TriangleStrip;
	lutPSODesc.VertexShader.ByteCode = lutVS.GetDataPtr();
	lutPSODesc.VertexShader.ByteCodeSize = lutVS.GetDataSize();
	lutPSODesc.PixelShader.ByteCode = lutPS.GetDataPtr();
	lutPSODesc.PixelShader.ByteCodeSize = lutPS.GetDataSize();

	PipelineStateObject lutPSO = {};
	if (!myRHI.CreatePipelineStateObject(lutPSODesc, lutPSO))
	{
		GELOG(Error, "Failed to create LUT PSO");
		return false;
	}

	{
		GraphicsCommandList lutCommandList = {};
		CreateCommandList("LUT", lutCommandList);

		const Texture* lut = &myLut;
		lutCommandList.SetRenderTarget(&lut, 1, nullptr);

		lutCommandList.SetPipelineState(lutPSO);
		lutCommandList.Draw(4);
		lutCommandList.FinishCommandList();
		ExecuteCommandList(lutCommandList);
	}

	return true;
}

void GraphicsEngine::ResetPipeline(GraphicsCommandList& inoutCommandList)
{
	inoutCommandList.BeginEvent("Reset Pipeline");

	inoutCommandList.ClearRenderTarget(myBackBuffer);
	inoutCommandList.ClearRenderTarget(myHDRBuffer);
	inoutCommandList.ClearDepthStencilView(myDepthBuffer);

	std::vector<Sampler*> samplerList(mySamplers.size());
	for (size_t s = 0; s < mySamplers.size(); ++s)
	{
		samplerList[s] = &mySamplers[s];
	}

	inoutCommandList.SetShaderSamplers(samplerList.data(), samplerList.size(), 0, PipelineStage_VertexShader | PipelineStage_PixelShader);

	inoutCommandList.EndEvent(); // Reset Pipeline
}

std::vector<Light> GraphicsEngine::SetupLights(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot)
{
	inoutCommandList.BeginEvent("Setup Lights");

	std::vector<Light> lights;
	LightBuffer lb;

	const CU::Matrix4f NDCtoUV = CU::Matrix4f::CreateNDCToTextureSpace();

	inoutCommandList.SetOverridePipelineState(*myShadowPSO, PipelineStage_None);

	unsigned spotLightShadowStartSlot = 110;
	unsigned pointLightShadowStartSlot = 120;

	for (size_t i = 0; i < aSnapshot.Lights.size(); ++i)
	{
		const LightComponent* lightComponent = aSnapshot.Lights[i];

		Light light;
		light.Color = lightComponent->GetColor();
		light.Intensity = lightComponent->GetIntensity();
		light.Position = lightComponent->GetOwner()->GetTransform().GetWorldPosition();
		light.Type = static_cast<uint32_t>(lightComponent->GetLightType());
		light.Direction = lightComponent->GetOwner()->GetTransform().GetLocalForward();
		light.InnerCone = lightComponent->GetInnerCone();
		light.OuterCone = lightComponent->GetOuterCone();
		light.Radius = lightComponent->GetRadius();
		light.CastShadows = lightComponent->CastShadows();

		if (lightComponent->CastShadows())
		{
			switch (lightComponent->GetLightType())
			{
				case LightType::Directional:
				{
					inoutCommandList.ChangeActiveOverrides(PipelineStage_PixelShader | PipelineStage_Rasterizer);

					const DirectionalLightComponent* directionalLight = static_cast<const DirectionalLightComponent*>(lightComponent);
					constexpr unsigned cascadeCount = DirectionalLightComponent::ShadowCascadeCount;
					const Texture* cascades[cascadeCount] = {};
					for (size_t cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex)
					{
						cascades[cascadeIndex] = RenderShadowCascade(inoutCommandList, aSnapshot, directionalLight, light, cascadeIndex);
						light.LightViewProjTexture[cascadeIndex] *= NDCtoUV;
					}

					inoutCommandList.SetShaderResources(cascades, cascadeCount, 100, PipelineStage_VertexShader | PipelineStage_PixelShader);
					break;
				}

				case LightType::Point:
				{
					inoutCommandList.ChangeActiveOverrides(PipelineStage_PixelShader | PipelineStage_Rasterizer | PipelineStage_GeometryShader);

					lb.Lights[0].Position = lightComponent->GetTransform().GetWorldPosition();
					UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, lb, 3, PipelineStage_GeometryShader);

					light.LightViewProjTexture[0] = RenderShadowMap(inoutCommandList, aSnapshot, lightComponent, pointLightShadowStartSlot++, PipelineStage_VertexShader | PipelineStage_GeometryShader) * NDCtoUV;
					break;
				}

				case LightType::Spot:
				{
					inoutCommandList.ChangeActiveOverrides(PipelineStage_PixelShader | PipelineStage_Rasterizer);
					light.LightViewProjTexture[0] = RenderShadowMap(inoutCommandList, aSnapshot, lightComponent, spotLightShadowStartSlot++, PipelineStage_VertexShader) * NDCtoUV;
					break;
				}
			}
		}

		lights.emplace_back(light);
	}

	inoutCommandList.ClearOverridePipelineState();
	inoutCommandList.ClearDepthStencilView(myDepthBuffer);

	inoutCommandList.EndEvent(); //Setup Lights

	return lights;
}

void GraphicsEngine::SortMeshObjects(const RenderSnapshot& aSnapshot, const CU::Vector3f& aCameraPosition, MeshList& outOpaqueMeshes, MeshList& outBlendedMeshes)
{
	outOpaqueMeshes.reserve(aSnapshot.Meshes.size());
	outBlendedMeshes.reserve(aSnapshot.Meshes.size());

	for (MeshComponentBase* meshComponent : aSnapshot.Meshes)
	{
		const std::shared_ptr<Mesh>& mesh = meshComponent->GetMesh()->GetMesh();
		const std::vector<std::shared_ptr<MaterialAsset>>& materials = meshComponent->GetMaterialList();

		bool addedOpaque = false;
		bool addedBlended = false;

		MaterialInterface* currentMaterial = nullptr;
		for (const Mesh::Element& element : mesh->myElements)
		{
			MaterialInterface* elementMaterial = materials[element.MaterialIndex] == nullptr ? nullptr : materials[element.MaterialIndex]->GetMaterial().get();
			if (elementMaterial != nullptr && elementMaterial != currentMaterial)
			{
				currentMaterial = elementMaterial;
			}
			else if (elementMaterial == nullptr)
			{
				currentMaterial = &myDefaultMaterial;
			}

			switch (currentMaterial->GetBlendMode())
			{
				case BlendMode::Opaque:
					if (!addedOpaque)
					{
						outOpaqueMeshes.emplace_back(meshComponent);
						addedOpaque = true;
					}
					break;

				default:
					if (!addedBlended)
					{
						outBlendedMeshes.emplace_back(meshComponent);
						addedBlended = true;
					}
					break;
			}
		}
	}

	std::sort(outOpaqueMeshes.begin(), outOpaqueMeshes.end(), [&aCameraPosition](const MeshComponentBase* aLeft, const MeshComponentBase* aRight)
		{
			const float distanceToLeft = (aCameraPosition - aLeft->GetTransform().GetWorldPosition()).LengthSqr();
			const float distanceToRight = (aCameraPosition - aRight->GetTransform().GetWorldPosition()).LengthSqr();
			return distanceToLeft < distanceToRight;
		});

	std::sort(outBlendedMeshes.begin(), outBlendedMeshes.end(), [&aCameraPosition](const MeshComponentBase* aLeft, const MeshComponentBase* aRight)
		{
			const float distanceToLeft = (aCameraPosition - aLeft->GetTransform().GetWorldPosition()).LengthSqr();
			const float distanceToRight = (aCameraPosition - aRight->GetTransform().GetWorldPosition()).LengthSqr();
			return distanceToLeft > distanceToRight;
		});
}

const Texture** GraphicsEngine::CreateDeferredTextures(GraphicsCommandList& inoutCommandList, const FrameBuffer& aFb, const MeshList aOpaqueMeshes)
{
	inoutCommandList.BeginEvent("Create Deferred Textures");

	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, aFb, 0, PipelineStage_VertexShader | PipelineStage_PixelShader);

	const Texture** gBuffer = myGBuffer.GetTexturePointers();
	inoutCommandList.SetRenderTarget(gBuffer, GBuffer::NumGBufferTargets, &myDepthBuffer);

	for (const MeshComponentBase* object : aOpaqueMeshes)
	{
		RenderMesh(inoutCommandList, object, 1);
	}

	inoutCommandList.EndEvent(); // Create Deferred Textures

	return gBuffer;
}

void GraphicsEngine::RunPreLightPostProcess(GraphicsCommandList& inoutCommandList, const Texture** aGBuffer, const GraphicsRenderSettings& aSettings)
{
	inoutCommandList.BeginEvent("Post Processing (Pre Light)");

	inoutCommandList.SetShaderResources(aGBuffer, GBuffer::NumGBufferTargets, 0, PipelineStage_VertexShader | PipelineStage_PixelShader);
	const Texture* plainWhite = &(*myPlainWhiteTexture);
	inoutCommandList.SetShaderResources(&plainWhite, 1, 80, PipelineStage_PixelShader);

	if (aSettings.RunPostProcesses)
	{
		for (size_t i = 0; i < myPostProcessEffects.size(); ++i)
		{
			if (!myPostProcessEffects[i]->IsActive())
			{
				continue;
			}

			myPostProcessEffects[i]->SetBuffers(inoutCommandList, *this);
			myPostProcessEffects[i]->RunPreLight(inoutCommandList, myPostProcessCache, &myHDRBuffer);
		}
	}

	inoutCommandList.EndEvent(); // Post Processing (Pre Light)
}

void GraphicsEngine::RenderDeferredLight(GraphicsCommandList& inoutCommandList, const GraphicsRenderSettings& aSettings, const Texture** aGBuffer,
	LightBuffer& aLb, const std::vector<Light>& aLights)
{
	inoutCommandList.BeginEvent("Render Deferred Light");

	inoutCommandList.SetShaderResources(aGBuffer, GBuffer::NumGBufferTargets, 0, PipelineStage_VertexShader | PipelineStage_PixelShader);

	inoutCommandList.SetPipelineState(*myAmbientLightPSO);
	inoutCommandList.Draw(4);

	if (aSettings.RenderPass == RenderPassType::Light)
	{
		for (const Light& light : aLights)
		{
			PipelineStateObject* desiredPSO = nullptr;
			switch (light.Type)
			{
				case 0:
					desiredPSO = myDirLightPSO.get();
					break;
				case 1:
					desiredPSO = myPointLightPSO.get();
					break;
				case 2:
					desiredPSO = mySpotLightPSO.get();
					break;
				default:
					GELOG(Warning, "Invalid light type {}!", light.Type);
					continue;
			}

			aLb.Lights[0] = light;
			UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, aLb, 3, PipelineStage_PixelShader);

			inoutCommandList.SetPipelineState(*desiredPSO);
			inoutCommandList.Draw(4);
		}
	}

	for (size_t i = 0; i < GBuffer::NumGBufferTargets; ++i)
	{
		inoutCommandList.ClearRenderTarget(myGBuffer.GetTextures()[i]);
	}

	inoutCommandList.EndEvent(); // Render Deferred Light
}

void GraphicsEngine::RenderBlendedObjects(GraphicsCommandList& inoutCommandList, const MeshList aBlendedMeshes,
	LightBuffer& aLb, const std::vector<Light>& aLights)
{
	inoutCommandList.BeginEvent("Render Blended Objects");

	inoutCommandList.SetRenderTarget(&myHDRBuffer, &myDepthBuffer);

	uint32_t activeLights = std::min<uint32_t>(static_cast<uint32_t>(aLights.size()), 8u);
	for (size_t i = 0; i < activeLights; ++i)
	{
		aLb.Lights[i] = aLights[i];
	}
	aLb.NumActiveLights = activeLights;

	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, aLb, 3, PipelineStage_PixelShader);

	for (const MeshComponentBase* object : aBlendedMeshes)
	{
		RenderMesh(inoutCommandList, object, 2);
	}

	inoutCommandList.EndEvent(); // Render Blended Objects
}

void GraphicsEngine::RunPostLightPostProcess(GraphicsCommandList& inoutCommandList, const GraphicsRenderSettings& aSettings)
{
	inoutCommandList.BeginEvent("Post Processing (Post Light)");

	if (aSettings.RunPostProcesses)
	{
		for (size_t i = 0; i < myPostProcessEffects.size(); ++i)
		{
			if (!myPostProcessEffects[i]->IsActive())
			{
				continue;
			}

			myPostProcessEffects[i]->SetBuffers(inoutCommandList, *this);
			myPostProcessEffects[i]->RunPostLight(inoutCommandList, myPostProcessCache, &myHDRBuffer);
		}
	}

	inoutCommandList.EndEvent(); // Post Processing (Post Light)
}

void GraphicsEngine::RunTonemap(GraphicsCommandList& inoutCommandList)
{
	inoutCommandList.BeginEvent("Tonemap");

	inoutCommandList.SetRenderTarget(&myBackBuffer, nullptr);

	inoutCommandList.SetShaderResource(&myHDRBuffer, 0, PipelineStage_PixelShader);

	inoutCommandList.SetPipelineState(*myTonemapPSO);
	inoutCommandList.Draw(4);

	inoutCommandList.EndEvent(); // Tonemap
}

void GraphicsEngine::RenderScreenText(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot)
{
	inoutCommandList.BeginEvent("Text Rendering");

	const CU::Vector2u clientSize = GetClientSize();

	inoutCommandList.SetRenderTarget(&myBackBuffer, nullptr);
	inoutCommandList.SetPipelineState(*myTextOverlayPSO);

	for (const std::shared_ptr<TextWidget>& text : aSnapshot.ScreenTextItems)
	{
		if (!text || !text->RebuildGeometry() || text->myIndices.empty() || !text->myFont || !text->myFont->GetAtlas())
		{
			continue;
		}

		if (text->myGpuDirty)
		{
			if (!myRHI.CreateVertexBuffer("TextWidget", text->myVertices, text->myVertexBuffer) ||
				!myRHI.CreateIndexBuffer("TextWidget", text->myIndices, text->myIndexBuffer))
			{
				continue;
			}
			text->myGpuDirty = false;
		}

		TextOverlayBuffer buffer;
		buffer.ClientSize = { static_cast<float>(clientSize.x), static_cast<float>(clientSize.y) };
		buffer.PixelOrigin = text->GetPosition();
		buffer.Opacity = text->GetOpacity();
		buffer.DistanceRange = text->myFont->GetDistanceRange();
		buffer.AtlasSize = { static_cast<float>(text->myFont->GetAtlasWidth()), static_cast<float>(text->myFont->GetAtlasHeight()) };

		UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::TextOverlayBuffer, buffer, 6, PipelineStage_VertexShader | PipelineStage_PixelShader);

		inoutCommandList.SetVertexBuffer(&text->myVertexBuffer);
		inoutCommandList.SetIndexBuffer(&text->myIndexBuffer);

		const Texture* atlas = text->myFont->GetAtlas().get();
		inoutCommandList.SetShaderResources(&atlas, 1, 0, PipelineStage_PixelShader);
		inoutCommandList.DrawIndexed(static_cast<unsigned>(text->myIndices.size()), 0);
	}

	inoutCommandList.EndEvent(); // Text Rendering
}

void GraphicsEngine::RenderScreenSprites(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot)
{
	if (aSnapshot.ScreenSpriteItems.empty())
	{
		return;
	}

	inoutCommandList.BeginEvent("Render sprites");

	constexpr float designWidth = 1920.0f;
	constexpr float designHeight = 1080.0f;

	CU::Vector2u windowSize = GetClientSize();

	float scaleX = windowSize.x / designWidth;
	float scaleY = windowSize.y / designHeight;
	float scale = CU::Math::Min(scaleX, scaleY);

	CU::Matrix4f projection = CU::Matrix4f{};
	projection(1, 1) = (2.0f / designWidth) * (scale / scaleX);
	projection(1, 4) = -1.0f;
	projection(2, 2) = (2.0f / designHeight) * (scale / scaleY);
	projection(2, 4) = -1.0f;
	projection(3, 3) = -1.0f;
	projection(4, 4) = 1.0f;

	FrameBuffer fb;
	fb.Projection = projection;
	fb.TotalTime = aSnapshot.TotalTime;
	inoutCommandList.SetRenderTarget(&myBackBuffer, nullptr);
	inoutCommandList.SetPipelineState(*mySpritePSO);

	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, fb, 0, PipelineStage_VertexShader | PipelineStage_PixelShader);

	for (size_t i = 0; i < aSnapshot.ScreenSpriteItems.size(); ++i)
	{
		const CU::Vector2f parentSize = { 1920.0f, 1080.0f };
		const CommonUtilities::Vector2f position =
		{
			parentSize.x *
			aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Anchor.x +
			aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Position.x -
			aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Size.x *
			aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Pivot.x,

			parentSize.y *
			aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Anchor.y +
			aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Position.y -
			aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Size.y *
			aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Pivot.y
		};

		const CommonUtilities::Vector2f& size = aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Size;
		const float left = position.x;
		const float bottom = position.y;
		const float right = left + size.x;
		const float top = bottom + size.y;

		Vertex v0 =
		{
			.Position = { left, top, aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Depth, 1.0f },
			.Color = aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Color,
			.UV0 = { 0.0f, 0.0f }
		};
		Vertex v1 =
		{
			.Position = { right, top, aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Depth, 1.0f },
			.Color = aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Color,
			.UV0 = { 1.0f, 0.0f }
		};
		Vertex v2 =
		{
			.Position = { left,  bottom, aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Depth, 1.0f },
			.Color = aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Color,
			.UV0 = { 0.0f, 1.0f },
		};
		Vertex v3 =
		{
			.Position = { right, bottom, aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Depth, 1.0f },
			.Color = aSnapshot.ScreenSpriteItems[i].SpriteInstanceData->Color,
			.UV0 = { 1.0f, 1.0f }
		};

		constexpr unsigned vertexCount = 6;
		Vertex vertices[vertexCount] = {
			v0,
			v1,
			v2,
			v1,
			v3,
			v2,
		};

		const Texture* textures = { aSnapshot.ScreenSpriteItems[i].SpriteSharedData->Texture.get() };
		inoutCommandList.SetShaderResources(&textures, 1, 0, PipelineStage_PixelShader);

		const MaterialInterface* material = aSnapshot.ScreenSpriteItems[i].Material;
		if (material == nullptr)
		{
			material = &myDefaultMaterial;
		}

		if (material->HasParameters())
		{
			if (material->IsMaterialDataDirty())
			{
				material->RefreshMaterialData();
			}

			UpdateAndSetConstantBufferInternal(inoutCommandList, ConstantBuffer::MaterialBuffer, material->GetParameterDataBlock(),
				Material::MATERIAL_BUFFER_SIZE, 4, PipelineStage_VertexShader | PipelineStage_PixelShader);
		}

		inoutCommandList.UpdateDynamicVertexBuffer(mySpriteVertexBuffer, vertices, vertexCount);
		inoutCommandList.SetVertexBuffer(&mySpriteVertexBuffer);
		inoutCommandList.Draw(vertexCount);
	}

	inoutCommandList.EndEvent();
}

void GraphicsEngine::RenderLines(GraphicsCommandList& inoutCommandList, const FrameBuffer& aFb)
{
	inoutCommandList.BeginEvent("Line Render Pass");

	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, aFb, 0, PipelineStage_VertexShader | PipelineStage_PixelShader);

	inoutCommandList.SetShaderResource(nullptr, 1, PipelineStage_PixelShader);
	inoutCommandList.SetRenderTarget(&myBackBuffer, nullptr);
	inoutCommandList.SetPipelineState(*myLinePSO);
	const std::vector<SimpleVertex>& lines = LineDrawer::Get().GetLines();
	{
		if (lines.size() != 0)
		{
			const int vertCount = static_cast<int>(lines.size());
			for (int i = 0; i < vertCount; i += 2)
			{
				int remaining = vertCount - i;
				int numVertsToDraw = CU::Math::Min(remaining, 2);
				inoutCommandList.UpdateDynamicVertexBuffer(myLineBuffer, lines.data() + i, numVertsToDraw * sizeof(SimpleVertex));
				inoutCommandList.SetVertexBuffer(&myLineBuffer);
				inoutCommandList.Draw(static_cast<unsigned>(numVertsToDraw));
			}
		}

		LineDrawer::Get().Clear();
	}
	inoutCommandList.EndEvent();
}

const Texture* GraphicsEngine::RenderShadowCascade(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot,
	const DirectionalLightComponent* aDirectionalLight, Light& aLight, size_t aCascadeIndex)
{
	inoutCommandList.BeginEvent("Render Shadow Cascade");

	const CU::Matrix4f cameraView = aSnapshot.ActiveCamera->GetView();
	const CU::Matrix4f& cascadeProj = aSnapshot.ActiveCamera->GetProjectionByCascadeIndex(aCascadeIndex);
	const CU::Matrix4f cascadeViewProj = cameraView * cascadeProj;
	const CU::Matrix4f invViewProj = cascadeViewProj.GetInverse();

	constexpr size_t cornerCount = 8;
	CU::Vector3f corners[cornerCount];
	size_t cornerIndex = 0;
	for (int x = 0; x < 2; ++x)
	{
		for (int y = 0; y < 2; ++y)
		{
			for (int z = 0; z < 2; ++z)
			{
				const CU::Vector4f pt = CU::Vector4f(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f) * invViewProj;
				corners[cornerIndex++] = pt.XYZ() * (1.0f / pt.w);
			}
		}
	}

	CU::Vector3f center = CU::Vector3f::Zero;
	for (size_t i = 0; i < cornerCount; ++i)
	{
		center += corners[i];
	}
	center *= 1.f / cornerCount;

	float radius = 0.0f;
	for (size_t i = 0; i < cornerCount; ++i)
	{
		radius = (std::max)(radius, (corners[i] - center).Length());
	}
	radius = std::ceil(radius);

	const CU::Vector3f up = CU::Vector3f(0, 1, 0);

	float texelsPerUnit = aDirectionalLight->GetShadowCascadeMap(aCascadeIndex)->GetWidth() / (radius * 2.0f);
	CU::Matrix4f lightView = CU::Matrix4f::LookAt(center - aLight.Direction * radius, center, up);

	CU::Vector4f shadowOrigin = CU::Vector4f(0, 0, 0, 1) * lightView;
	shadowOrigin *= texelsPerUnit;
	const CU::Vector4f rounded = CU::Vector4f(std::round(shadowOrigin.x), std::round(shadowOrigin.y), shadowOrigin.z, shadowOrigin.w);
	const CU::Vector4f offset = (rounded - shadowOrigin) * (1.0f / texelsPerUnit);

	lightView(4, 1) += offset.x;
	lightView(4, 2) += offset.y;

	const CU::Matrix4f lightProj = CU::Matrix4f::CreateOrthographicProjection(-radius, radius, -radius, radius, -radius * 4.0f, radius * 4.0f);

	FrameBuffer lightFb;
	lightFb.View = lightView;
	lightFb.Projection = lightProj;
	lightFb.TotalTime = aSnapshot.TotalTime;
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, lightFb, 0, PipelineStage_VertexShader);

	const Texture& shadowMap = *aDirectionalLight->GetShadowCascadeMap(aCascadeIndex);
	inoutCommandList.ClearDepthStencilView(shadowMap);
	inoutCommandList.SetRenderTarget(nullptr, 0, &shadowMap);

	for (const MeshComponentBase* meshComponent : aSnapshot.Meshes)
	{
		RenderMesh(inoutCommandList, meshComponent, 0);
	}

	inoutCommandList.SetRenderTarget(nullptr, 0, nullptr);

	aLight.LightViewProjTexture[aCascadeIndex] = lightView * lightProj;

	inoutCommandList.EndEvent(); // Render Shadow Cascade

	return &shadowMap;
}

CU::Matrix4f GraphicsEngine::RenderShadowMap(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot,
	const LightComponent* aLight, unsigned aStartSlot, PipelineStages aFrameBufferStages)
{
	inoutCommandList.BeginEvent("Update Shadowmaps");

	FrameBuffer lightFb;
	lightFb.View = aLight->GetOwner()->GetTransform().GetWorldMatrix().GetFastInverse();
	lightFb.Projection = aLight->GetProjectionMatrix();
	lightFb.TotalTime = aSnapshot.TotalTime;
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, lightFb, 0, aFrameBufferStages);

	const Texture& shadowMap = *aLight->GetShadowMap();
	inoutCommandList.ClearDepthStencilView(shadowMap);
	inoutCommandList.SetRenderTarget(nullptr, 0, &shadowMap);

	for (const MeshComponentBase* meshComponent : aSnapshot.Meshes)
	{
		RenderMesh(inoutCommandList, meshComponent, 0);
	}

	inoutCommandList.SetRenderTarget(nullptr, 0, nullptr);

	const Texture* textures[1]{ &shadowMap };
	inoutCommandList.SetShaderResources(textures, 1, aStartSlot, PipelineStage_VertexShader | PipelineStage_PixelShader);

	inoutCommandList.EndEvent(); // Update Shadowmaps

	return lightFb.View * lightFb.Projection;
}

bool GraphicsEngine::PrepareMeshForRendering(const Mesh& aMesh) const
{
	if (!aMesh.myVertexBuffer.IsValid())
	{
		if (!myRHI.CreateVertexBuffer(aMesh.myName, aMesh.myVertices, aMesh.myVertexBuffer))
		{
			GELOG(Warning, "Could not prepare mesh {}! Vertex Buffer could not be created!", aMesh.myName);
			return false;
		}
	}

	if (!aMesh.myIndexBuffer.IsValid())
	{
		if (!myRHI.CreateIndexBuffer(aMesh.myName, aMesh.myIndices, aMesh.myIndexBuffer))
		{
			GELOG(Warning, "Could not prepare mesh {}! Index Buffer could not be created!", aMesh.myName);
			return false;
		}
	}

	return true;
}

void GraphicsEngine::RenderMesh(GraphicsCommandList& inoutCommandList, const MeshComponentBase* aWorldObject, char aOpaqueOrBlended)
{
	const std::shared_ptr<Mesh>& mesh = aWorldObject->GetMesh()->GetMesh();
	const std::vector<std::shared_ptr<MaterialAsset>>& materials = aWorldObject->GetMaterialList();

	if (!mesh || !PrepareMeshForRendering(*mesh))
	{
		return;
	}

	inoutCommandList.SetVertexBuffer(&mesh->myVertexBuffer);
	inoutCommandList.SetIndexBuffer(&mesh->myIndexBuffer);

	ObjectBuffer ob;
	ob.World = aWorldObject->GetOwner()->GetTransform().GetWorldMatrix();
	ob.WorldInvT = ob.World.GetInverse().GetTranspose();
	ob.HasSkinning = mesh->HasSkeleton();
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::ObjectBuffer, ob, 1, PipelineStage_VertexShader);

	if (ob.HasSkinning)
	{
		AnimationBuffer ab;
		memcpy_s(ab.JointAnimation, sizeof(CU::Matrix4f) * 128, aWorldObject->GetJointTransforms()->data(), sizeof(CU::Matrix4f) * 128);
		UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::AnimationBuffer, ab, 2, PipelineStage_VertexShader);
	}

	MaterialInterface* currentMaterial = nullptr;
	for (const Mesh::Element& element : mesh->myElements)
	{
		MaterialInterface* elementMaterial = materials[element.MaterialIndex] == nullptr ? nullptr : materials[element.MaterialIndex]->GetMaterial().get();
		if (elementMaterial != nullptr && elementMaterial != currentMaterial)
		{
			currentMaterial = elementMaterial;
		}
		else if (elementMaterial == nullptr)
		{
			currentMaterial = &myDefaultMaterial;
		}

		if (aOpaqueOrBlended != 0 &&
			(aOpaqueOrBlended != 1 || currentMaterial->GetBlendMode() != BlendMode::Opaque) &&
			(aOpaqueOrBlended != 2 || currentMaterial->GetBlendMode() == BlendMode::Opaque))
		{
			continue;
		}

		if (currentMaterial->HasParameters())
		{
			if (currentMaterial->IsMaterialDataDirty())
			{
				currentMaterial->RefreshMaterialData();
			}

			UpdateAndSetConstantBufferInternal(inoutCommandList, ConstantBuffer::MaterialBuffer, currentMaterial->GetParameterDataBlock(), Material::MATERIAL_BUFFER_SIZE, 4, PipelineStage_VertexShader | PipelineStage_PixelShader);
		}

		std::vector<const Texture*> textures(Material::MAX_MATERIAL_TEXTURE_COUNT);
		for (size_t t = 0; t < Material::MAX_MATERIAL_TEXTURE_COUNT; ++t)
		{
			if (const std::shared_ptr<Texture>& texture = currentMaterial->GetTexture(static_cast<unsigned>(t)))
			{
				textures[t] = texture.get();
			}
		}

		inoutCommandList.SetShaderResources(textures.data(), textures.size(), 0, PipelineStage_VertexShader | PipelineStage_PixelShader);

		inoutCommandList.SetPipelineState(currentMaterial->GetPSO());
		inoutCommandList.DrawIndexed(element.NumIndices, element.IndexOffset);
	}
}
