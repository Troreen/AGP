#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include "RHI/RenderHardwareInterface.h"
#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/LightBuffer.h"
#include "Objects/Buffer.h"
#include "Objects/Texture.h"
#include "Objects/Vertex.h"
#include "Objects/Mesh.h"
#include "Objects/Sampler.h"

#include "Camera3D.hpp"
#include "Matrix.hpp"

#include "RHI/PipelineStateObject.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

class Actor;
class MeshComponentBase;
class World;
enum class LightType : uint32_t;

struct RHIShaderReflectionInfo;
struct MaterialDescription;

enum class ConstantBuffer : uint8_t
{
	FrameBuffer,
	ObjectBuffer,
	AnimationBuffer,
	MaterialBuffer,
	LightBuffer,
	PointShadowBuffer,
	MAX
};

class GraphicsEngine
{
public:

	static GraphicsEngine& Get();

	bool Initialize(HWND aWindowHandle, const std::filesystem::path& aShaderRoot);
	void Render(GraphicsCommandList& inoutCommandList, const Actor& aCameraActor, const World& aWorld);
	void Present() const;

	template <class T>
	bool CreateConstantBuffer(ConstantBuffer aBufferId, std::string_view aName) 
	{
		return CreateConstantBufferInternal(aBufferId, aName, sizeof(T));
	}

	bool CreateConstantBuffer(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize);


	template <class T>
	bool UpdateAndSetConstantBuffer(GraphicsCommandList& inoutCommandList, ConstantBuffer aBufferId, const T& aData, unsigned aSlot, PipeLineStages aStages)
	{
		return UpdateAndSetConstantBufferInternal(inoutCommandList, aBufferId, &aData, sizeof(T), aSlot, aStages);
	}

	CU::Vector2u GetClientSize() const;
	
	bool CreateCommandList(std::string_view aName, GraphicsCommandList& outCommandList) const;
	void ExecuteCommandList(const GraphicsCommandList& aCommandList) const;

	bool CreateMaterial(const MaterialDescription& aDescription, Material& outMaterial) const;

	bool LoadTexture(const std::filesystem::path& aPath, Texture& outTexture) const;

	bool CreateShadowMap(std::string_view aName, unsigned aWidth, unsigned aHeight, Texture& outShadowMap, bool aCubeMap = false) const;
	void AdjustShadowBias(LightType aType, float aDelta);
	void ResetShadowTuning();
	void LogShadowTuning() const;
private:

	struct RenderItem
	{
		const MeshComponentBase* MeshComponent = nullptr;
		CU::Matrix4f World;
	};

	struct SceneRenderData
	{
		std::vector<RenderItem> RenderItems;
		std::vector<const class LightComponent*> LightComponents;
	};

	static constexpr unsigned DirectionalCascadeCount = 4;
	static constexpr unsigned MaxSpotShadowMaps = 4;
	static constexpr unsigned MaxPointShadowMaps = 4;

	bool CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize);
	bool UpdateAndSetConstantBufferInternal(GraphicsCommandList& inoutCommandList, ConstantBuffer aBufferId, const void* aData, size_t aDataSize, unsigned aSlot, PipeLineStages aStages);

	void CreateMaterialTextureSlots(const RHIShaderReflectionInfo& aShaderInfo, Material& inoutMaterial) const;
	bool CreateShadowResources();
	bool CreateShadowPipelineStates();
	SceneRenderData CollectRenderItemsAndLights(const World& aWorld) const;
	void UnbindShadowResources(GraphicsCommandList& inoutCommandList) const;
	void BindShadowResources(GraphicsCommandList& inoutCommandList) const;
	void RenderShadowMap(GraphicsCommandList& inoutCommandList, std::string_view aEventName, Texture& aShadowMap, const FrameBuffer& aFrameBuffer, const PipelineStateObject& aOverridePSO, PipeLineStages aOverrideStages, const void* aPointShadowBuffer, const std::vector<RenderItem>& aRenderItems);
	void RenderDirectionalShadows(GraphicsCommandList& inoutCommandList, const CU::Camera3D& aCamera, const class LightComponent& aLightComponent, LightBuffer::Light& inoutLight, const std::vector<RenderItem>& aRenderItems);
	void RenderSpotShadows(GraphicsCommandList& inoutCommandList, const class LightComponent& aLightComponent, LightBuffer::Light& inoutLight, unsigned aShadowIndex, const std::vector<RenderItem>& aRenderItems);
	void RenderPointShadows(GraphicsCommandList& inoutCommandList, const class LightComponent& aLightComponent, LightBuffer::Light& inoutLight, unsigned aShadowIndex, const std::vector<RenderItem>& aRenderItems);
	float GetShadowDepthBias(LightType aType) const;

	GraphicsEngine();
	~GraphicsEngine();

	bool PrepareMeshForRendering(const Mesh& aMesh) const;
	void RenderMesh(GraphicsCommandList& inoutCommandList, const MeshComponentBase& aMeshComponent, const CU::Matrix4f& aWorld);
	bool CreateDefaultTextures();

	RenderHardwareInterface myRHI;
	Texture myBackBuffer;
	Texture myDepthBuffer;

	std::unordered_map<ConstantBuffer, Buffer> myConstantBuffers;

	PipelineStateObject myDefaultSurfacePSO;
	PipelineStateObject myShadowOverridePSO;
	PipelineStateObject myLocalShadowOverridePSO;
	PipelineStateObject myPointShadowOverridePSO;

	std::filesystem::path myShaderRoot;
	std::unordered_map<MaterialDomain, std::filesystem::path> myMaterialDomainShaders;
	std::unordered_map<ShadingModel, std::filesystem::path> myMaterialShadingModelShaders;

	std::vector<Sampler> mySamplers;
	std::array<Texture, DirectionalCascadeCount> myDirectionalShadowMaps;
	std::array<Texture, MaxSpotShadowMaps> mySpotShadowMaps;
	std::array<Texture, MaxPointShadowMaps> myPointShadowMaps;
	std::shared_ptr<Texture> myDefaultAlbedoTexture;
	std::shared_ptr<Texture> myDefaultNormalTexture;

	Material myDefaultMaterial;
	float myDirectionalShadowBiasOffset = 0.0f;
	float mySpotShadowBiasOffset = 0.0f;
	float myPointShadowBiasOffset = 0.0f;
};
