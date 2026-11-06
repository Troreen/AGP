#pragma once

#include "AmbientSettings.h"
#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/LightBuffer.h"
#include "GraphicsRenderSettings.h"
#include "Objects/Buffer.h"
#include "Objects/GBuffer.h"
#include "Objects/Mesh.h"
#include "Objects/Sampler.h"
#include "Objects/Texture.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "PostProcessing/PostProcessCache.h"
#include "PostProcessing/PostProcessEffect.h"
#include "RenderSnapshot.h"
#include "RHI/PipelineStateObject.h"
#include "RHI/RenderHardwareInterface.h"

#include <CommonUtilities/CommonUtilities.h>
#include <unordered_map>
#include <filesystem>

struct MaterialDescription;
struct RHIShaderReflectionInfo;

class DirectionalLightComponent;
class ParticleEmitter;

enum class ConstantBuffer : uint8_t
{
	FrameBuffer,
	LightBuffer,
	ObjectBuffer,
	AnimationBuffer,
	MaterialBuffer,
	PostProcessBuffer,
	TextOverlayBuffer,
	MAX
};

typedef struct HWND__ *HWND;

class GraphicsEngine
{
public:
	static constexpr unsigned MAX_MESH_INSTANCES = 128;

	static GraphicsEngine& Get();

	bool Initialize(HWND aWindowHandle, const std::filesystem::path& aShaderRoot);
	void Render(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot, const GraphicsRenderSettings& aSettings);
	void Present() const;

	void SetAmbientSettings(AmbientSettings&& aSettings);
	void AddPostProcessEffect(const std::shared_ptr<PostProcessEffect>& aEffect);
	void RemovePostProcessEffect(const std::shared_ptr<PostProcessEffect>& aEffect);
	void SetColorGradingLut(const std::filesystem::path& aLutPath);

	template<class T>
	bool CreateConstantBuffer(ConstantBuffer aBufferId, std::string_view aName)
	{
		return CreateConstantBufferInternal(aBufferId, aName, sizeof(T));
	}

	bool CreateConstantBuffer(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize);

	template<class T>
	bool UpdateAndSetConstantBuffer(GraphicsCommandList& inputCommandList, ConstantBuffer aBufferId, const T& aData, unsigned aSlot, PipelineStages aStages)
	{
		return UpdateAndSetConstantBufferInternal(inputCommandList, aBufferId, &aData, sizeof(T), aSlot, aStages);
	}

	CU::Vector2u GetClientSize() const;
	bool Resize(unsigned aWidth, unsigned aHeight);

	bool CreateCommandList(std::string_view aName, GraphicsCommandList& outCommandList) const;
	void ExecuteCommandList(const GraphicsCommandList& aCommandList) const;

	bool CreateMaterial(const MaterialDescription& aDescription, Material& outMaterial) const;

	bool LoadTexture(const std::filesystem::path& aPath, Texture& outTexture) const;
	bool LoadLutCubeTexture(const std::filesystem::path& aPath, Texture& outTexture) const;
	bool CreateTexture(std::string_view aName, unsigned aWidth, unsigned aHeight, unsigned aNumMips, ResourceFormat aFormat, ResourceBindFlags aBindFlags, Texture& outTexture) const;
	bool CreateColorTexture(std::string_view aName, const std::array<uint8_t, 4>& aColor, Texture& outTexture) const;
	bool CreateShadowMap(std::string_view aName, unsigned aWidth, unsigned aHeight, Texture& outShadowMap, bool aCube) const;

	ID3D11Device* GetNativeDevice() const;
	ID3D11DeviceContext* GetNativeDeviceContext() const;

	Texture* GetBackBuffer();
	void SetRenderTarget(const Texture* aTargetList, const Texture* aDepthStencil) const;

	const Material* GetDefaultMaterial() const { return &myDefaultMaterial; }

private:
	using MeshList = std::vector<MeshComponentBase*>;

	bool CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize);
	bool UpdateAndSetConstantBufferInternal(GraphicsCommandList& inputCommandList, ConstantBuffer aBufferId, const void* aData, size_t aDataSize, unsigned aSlot, PipelineStages aStages);

	void CreateMaterialTextureSlots(const RHIShaderReflectionInfo& aShaderInfo, Material& inoutMaterial) const;

	GraphicsEngine();
	~GraphicsEngine();

	bool CreatePSOs();
	void CreateSamplers();
	bool GenerateLUT();

	void ResetPipeline(GraphicsCommandList& inoutCommandList);
	std::vector<Light> SetupLights(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot, CU::Vector3f& outSunDirectionToLight);
	const Texture** CreateDeferredTextures(GraphicsCommandList& inoutCommandList, const FrameBuffer& aFb, const MeshList aOpaqueMeshes);
	void RunPreLightPostProcess(GraphicsCommandList& inoutCommandList, const Texture** aGBuffer, const GraphicsRenderSettings& aSettings);
	void RenderDeferredLight(GraphicsCommandList& inoutCommandList, const GraphicsRenderSettings& aSettings, const Texture** aGBuffer,
		LightBuffer& aLb, const std::vector<Light>& aLights);
	void RenderSkybox(GraphicsCommandList& inoutCommandList, const GraphicsRenderSettings& aSettings);
	void RenderBlendedObjects(GraphicsCommandList& inoutCommandList, const MeshList aBlendedMeshes, LightBuffer& aLb, const std::vector<Light>& aLights);
	void RenderParticles(GraphicsCommandList& inoutCommandList, const std::vector<ParticleComponent*>& aParticles);
	void RunPostLightPostProcess(GraphicsCommandList& inoutCommandList, const GraphicsRenderSettings& aSettings);
	void RunTonemap(GraphicsCommandList& inoutCommandList);
	void RenderLines(GraphicsCommandList& inoutCommandList, const FrameBuffer& aFb);
	void RenderScreenText(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot);
	void RenderScreenSprites(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot);

	const Texture* RenderShadowCascade(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot,
		const DirectionalLightComponent* aDirectionalLight, Light& aLight, size_t aCascadeIndex);

	CU::Matrix4f RenderShadowMap(GraphicsCommandList& inoutCommandList, const RenderSnapshot& aSnapshot,
		const LightComponent* aLight, unsigned aStartSlot, PipelineStages aFrameBufferStages);

	bool PrepareMeshForRendering(const Mesh& aMesh) const;
	bool PrepareParticlemitterForRendering(const ParticleEmitter& aEmitter) const;
	void RenderMesh(GraphicsCommandList& inoutCommandList, const MeshComponentBase* aWorldObject, char aOpaqueOrBlended);

	AmbientSettings myAmbientSettings;

	GBuffer myGBuffer;
	Buffer mySpriteVertexBuffer;
	Buffer myLineBuffer;
	Buffer myInstanceBuffer;

	std::unique_ptr<PipelineStateObject> mySkyboxPSO;
	std::unique_ptr<PipelineStateObject> myAmbientLightPSO;
	std::unique_ptr<PipelineStateObject> myDirLightPSO;
	std::unique_ptr<PipelineStateObject> myPointLightPSO;
	std::unique_ptr<PipelineStateObject> mySpotLightPSO;
	std::unique_ptr<PipelineStateObject> myShadowPSO;
	std::unique_ptr<PipelineStateObject> myTonemapPSO;
	std::unique_ptr<PipelineStateObject> myTextOverlayPSO;
	std::unique_ptr<PipelineStateObject> mySpritePSO;
	std::unique_ptr<PipelineStateObject> myLinePSO;

	RenderHardwareInterface myRHI;
	Texture myBackBuffer;
	Texture myDepthBuffer;
	Texture myHDRBuffer;
	Texture myLut;
	Texture myColorGradingLut;

	Texture myDefaultColorGradingLut;
	Texture myDefaultEnvironmentCubeTexture;

	std::shared_ptr<Texture> myPlainWhiteTexture;
	std::shared_ptr<Texture> myFlatNormalTexture;
	std::shared_ptr<Texture> myDefaultORMTexture;
	std::shared_ptr<Texture> myNoEmissionTexture;

	std::unordered_map<ConstantBuffer, Buffer> myConstantBuffers;

	std::filesystem::path myShaderRoot;
	std::unordered_map<MaterialDomain, std::filesystem::path> myMaterialDomainShaders;
	std::unordered_map<ShadingModel, std::filesystem::path> myMaterialShadingModelShaders;

	std::vector<Sampler> mySamplers;
	std::vector<std::shared_ptr<PostProcessEffect>> myPostProcessEffects;
	PostProcessCache myPostProcessCache;
	
	Material myDefaultMaterial;
};
