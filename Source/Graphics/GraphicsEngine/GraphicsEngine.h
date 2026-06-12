#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include "RHI/RenderHardwareInterface.h"
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

struct RHIShaderReflectionInfo;
struct MaterialDescription;

enum class ConstantBuffer : uint8_t
{
	FrameBuffer,
	ObjectBuffer,
	AnimationBuffer,
	MaterialBuffer,
	LightBuffer,
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
private:

	bool CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize);
	bool UpdateAndSetConstantBufferInternal(GraphicsCommandList& inoutCommandList, ConstantBuffer aBufferId, const void* aData, size_t aDataSize, unsigned aSlot, PipeLineStages aStages);

	void CreateMaterialTextureSlots(const RHIShaderReflectionInfo& aShaderInfo, Material& inoutMaterial) const;

	GraphicsEngine();
	~GraphicsEngine();

	bool PrepareMeshForRendering(const Mesh& aMesh) const;
	void RenderMesh(GraphicsCommandList& inoutCommandList, const MeshComponentBase& aMeshComponent, const CU::Matrix4f& aWorld);
	bool CreateDefaultTextures();

	RenderHardwareInterface myRHI;
	Texture myBackBuffer;
	Texture myDepthBuffer;

	std::unordered_map<ConstantBuffer, Buffer> myConstantBuffers;

	// TODO: Temporary PSO=
	PipelineStateObject myTempPSO;

	std::filesystem::path myShaderRoot;
	std::unordered_map<MaterialDomain, std::filesystem::path> myMaterialDomainShaders;
	std::unordered_map<ShadingModel, std::filesystem::path> myMaterialShadingModelShaders;

	std::vector<Sampler> mySamplers;
	std::shared_ptr<Texture> myDefaultAlbedoTexture;
	std::shared_ptr<Texture> myDefaultNormalTexture;

	Material myDefaultMaterial;
};
