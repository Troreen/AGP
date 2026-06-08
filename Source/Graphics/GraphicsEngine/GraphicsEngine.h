#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include "RHI/RenderHardwareInterface.h"
#include "Objects/Buffer.h"
#include "Objects/Texture.h"
#include "Objects/Vertex.h"
#include "Objects/Mesh.h"

#include "Camera3D.hpp"
#include "Matrix.hpp"

#include "PipelineStateObject.h"

class Actor;
class World;

enum class ConstantBuffer : uint8_t
{
	FrameBuffer,
	ObjectBuffer,
	MAX
};

class GraphicsEngine
{
public:

	static GraphicsEngine& Get();

	bool Initialize(HWND aWindowHandle);
	void Render(const Actor& aCameraActor, const World& aWorld);

	template <class T>
	bool CreateConstantBuffer(ConstantBuffer aBufferId, std::string_view aName) 
	{
		return CreateConstantBufferInternal(aBufferId, aName, sizeof(T));
	}

	template <class T>
	bool UpdateAndSetConstantBuffer(ConstantBuffer aBufferId, const T& aData, unsigned aSlot, PipeLineStages aStages)
	{
		return UpdateAndSetConstantBufferInternal(aBufferId, &aData, sizeof(T), aSlot, aStages);
	}

	CU::Vector2u GetClientSize() const;
	
private:

	bool CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize);
	bool UpdateAndSetConstantBufferInternal(ConstantBuffer aBufferId, const void* aData, size_t aDataSize, unsigned aSlot, PipeLineStages aStages);

	GraphicsEngine();
	~GraphicsEngine();

	bool PrepareMeshForRendering(const Mesh& aMesh) const;
	void RenderMesh(const Mesh& aMesh, const CU::Matrix4f& aWorld);

	RenderHardwareInterface myRHI;
	Texture myBackBuffer;
	Texture myDepthBuffer;

	std::unordered_map<ConstantBuffer, Buffer> myConstantBuffers;

	// TODO: Temporary PSO=
	PipelineStateObject myTempPSO;
};
