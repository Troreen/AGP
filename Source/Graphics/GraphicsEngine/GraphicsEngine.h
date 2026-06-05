#pragma once

#include <string>
#include <unordered_map>
#include <wrl.h>

#include "RHI/RenderHardwareInterface.h"
#include "Objects/Buffer.h"
#include "Objects/Texture.h"
#include "Objects/Vertex.h"
#include "Objects/Mesh.h"

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
	void Render(const Mesh& aMesh);

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
private:

	bool CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize);
	bool UpdateAndSetConstantBufferInternal(ConstantBuffer aBufferId, const void* aData, size_t aDataSize, unsigned aSlot, PipeLineStages aStages);

	GraphicsEngine();
	~GraphicsEngine();

	bool PrepareMeshForRendering(const Mesh& aMesh) const;

	RenderHardwareInterface myRHI;
	Texture myBackBuffer;

	std::unordered_map<ConstantBuffer, Buffer> myConstantBuffers;
};
