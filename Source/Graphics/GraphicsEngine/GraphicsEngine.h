#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include "RHI/RenderHardwareInterface.h"
#include "RHI/GraphicsCommandList.h"
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
class LightComponent;
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

	struct RenderItemSnapshot
	{
		std::shared_ptr<Mesh> Mesh;
		std::vector<std::shared_ptr<MaterialInterface>> Materials;
		CU::Matrix4f World;
		std::array<CU::Matrix4f, 128> JointTransforms = {};
		CU::Vector3f BoundsCenter = CU::Vector3f::Zero;
		float BoundsRadius = 0.0f;
		bool HasSkinning = false;
		bool HasBounds = false;
	};

	struct LightSnapshot
	{
		LightType Type{};
		CU::Vector3f Color = CU::Vector3f::One;
		float Intensity = 0.0f;
		CU::Vector3f Position = CU::Vector3f::Zero;
		CU::Vector3f Direction = CU::Vector3f::UnitZ;
		float InnerCone = 0.0f;
		float OuterCone = 0.0f;
		float Radius = 1.0f;
	};

	struct RenderStats
	{
		uint32_t TotalRenderItems = 0;
		uint32_t VisibleRenderItems = 0;
		uint32_t ShadowCasters = 0;
		uint32_t TotalLights = 0;
		uint32_t RelevantLights = 0;
		uint32_t DirectionalShadowPasses = 0;
		uint32_t SpotShadowPasses = 0;
		uint32_t PointShadowPasses = 0;
		uint32_t ShadowCasterDraws = 0;
		uint32_t CulledShadowCasters = 0;
		uint32_t ShadowCommandListsRecorded = 0;
		uint32_t ShadowCommandListsExecuted = 0;
	};

	struct RenderSceneSnapshot
	{
		bool HasCamera = false;
		CU::Camera3D Camera;
		std::vector<RenderItemSnapshot> ShadowCasters;
		std::vector<RenderItemSnapshot> VisibleRenderItems;
		std::vector<LightSnapshot> RelevantLights;
		RenderStats Stats;

		void Clear();
	};

	static GraphicsEngine& Get();

	bool Initialize(HWND aWindowHandle, const std::filesystem::path& aShaderRoot);
	void Render(GraphicsCommandList& inoutCommandList, const Actor& aCameraActor, const World& aWorld);
	bool BuildRenderSnapshot(const Actor& aCameraActor, const World& aWorld, RenderSceneSnapshot& outSnapshot) const;
	void RenderSnapshot(GraphicsCommandList& inoutCommandList, const RenderSceneSnapshot& aSnapshot);
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
	RenderStats GetLastRenderStats() const;

	bool CreateMaterial(const MaterialDescription& aDescription, Material& outMaterial) const;

	bool LoadTexture(const std::filesystem::path& aPath, Texture& outTexture) const;

	bool CreateShadowMap(std::string_view aName, unsigned aWidth, unsigned aHeight, Texture& outShadowMap, bool aCubeMap = false) const;
	void AdjustShadowBias(LightType aType, float aDelta);
	void ResetShadowTuning();
	void LogShadowTuning() const;
private:

	static constexpr unsigned DirectionalCascadeCount = 4;
	static constexpr unsigned MaxSpotShadowMaps = 4;
	static constexpr unsigned MaxPointShadowMaps = 4;

	bool CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize);
	bool UpdateAndSetConstantBufferInternal(GraphicsCommandList& inoutCommandList, ConstantBuffer aBufferId, const void* aData, size_t aDataSize, unsigned aSlot, PipeLineStages aStages);

	void CreateMaterialTextureSlots(const RHIShaderReflectionInfo& aShaderInfo, Material& inoutMaterial) const;
	bool CreatePBLResources();
	bool CreateBRDFLUT();
	void BindPBLResources(GraphicsCommandList& inoutCommandList) const;
	bool CreateShadowResources();
	bool CreateShadowPipelineStates();
	void UnbindShadowResources(GraphicsCommandList& inoutCommandList) const;
	void BindShadowResources(GraphicsCommandList& inoutCommandList) const;
	void PrepareSnapshotRenderResources(const RenderSceneSnapshot& aSnapshot) const;
	bool PrepareRenderItemResources(const RenderItemSnapshot& aRenderItem) const;
	bool EnsureShadowCommandListCount(size_t aCount);
	void StoreLastRenderStats(const RenderStats& aStats);
	void RenderShadowMap(GraphicsCommandList& inoutCommandList, std::string_view aEventName, Texture& aShadowMap, const FrameBuffer& aFrameBuffer, const PipelineStateObject& aOverridePSO, PipeLineStages aOverrideStages, const void* aPointShadowBuffer, const std::vector<const RenderItemSnapshot*>& aRenderItems);
	void RenderDirectionalShadows(GraphicsCommandList& inoutCommandList, const CU::Camera3D& aCamera, const LightSnapshot& aLight, LightBuffer::Light& inoutLight, const std::vector<RenderItemSnapshot>& aRenderItems);
	void RenderSpotShadows(GraphicsCommandList& inoutCommandList, const LightSnapshot& aLight, LightBuffer::Light& inoutLight, unsigned aShadowIndex, const std::vector<RenderItemSnapshot>& aRenderItems);
	void RenderPointShadows(GraphicsCommandList& inoutCommandList, const LightSnapshot& aLight, LightBuffer::Light& inoutLight, unsigned aShadowIndex, const std::vector<RenderItemSnapshot>& aRenderItems);
	float GetShadowDepthBias(LightType aType) const;
	float GetShadowDepthBiasUnlocked(LightType aType) const;

	GraphicsEngine();
	~GraphicsEngine();

	bool PrepareMeshForRendering(const Mesh& aMesh) const;
	void RenderMesh(GraphicsCommandList& inoutCommandList, const RenderItemSnapshot& aRenderItem, bool aAllowLazyPrepare = true);
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
	std::vector<const Sampler*> mySamplerBindings;
	std::vector<GraphicsCommandList> myShadowCommandLists;
	std::array<Texture, DirectionalCascadeCount> myDirectionalShadowMaps;
	std::array<Texture, MaxSpotShadowMaps> mySpotShadowMaps;
	std::array<Texture, MaxPointShadowMaps> myPointShadowMaps;
	Texture myEnvironmentCubeTexture;
	Texture myBRDFLUTTexture;
	std::shared_ptr<Texture> myDefaultAlbedoTexture;
	std::shared_ptr<Texture> myDefaultNormalTexture;
	std::shared_ptr<Texture> myDefaultMaterialTexture;

	Material myDefaultMaterial;
	mutable std::mutex myShadowTuningMutex;
	mutable std::mutex myRenderStatsMutex;
	RenderStats myLastRenderStats;
	float myDirectionalShadowBiasOffset = 0.0f;
	float mySpotShadowBiasOffset = 0.0f;
	float myPointShadowBiasOffset = 0.0f;
};
