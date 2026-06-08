#pragma once
#include <wrl.h>
#include <Windows.h>
#include <string_view>
#include <vector>

#include "RHIStructs.h"
#include "Vector.hpp"


class PipelineStateObject;
struct PipelineStateDescription;
class Buffer;
struct Vertex;
class Texture;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11DeviceChild;

class RenderHardwareInterface
{
public:	
	RenderHardwareInterface();
	~RenderHardwareInterface();

	bool Initialize(HWND aWindowHandle, bool aEnableDebug, Texture& outBackBuffer, Texture& outDepthStencil);

	void Present() const; 
	void ClearRenderTarget(const Texture& aTarget) const;
	void ClearDepthStencil(const Texture& aTarget) const;
	void SetRenderTarget(const Texture* aTarget, const Texture* aDepthStencil) const;

	CommonUtilities::Vector2u GetClientSize() const;

	bool CreateVertexBuffer(std::string_view aName, const std::vector<Vertex>&  aVertexList, Buffer& outBuffer) const;
	bool CreateIndexBuffer(std::string_view aName, const std::vector<unsigned>&  aIndexList, Buffer& outBuffer) const;
	bool CreateConstantBuffer(std::string_view aName, size_t aSize, Buffer& outBuffer) const;

	bool CreatePipelineStateObject (const PipelineStateDescription& aDescription, PipelineStateObject& outPSO) const;

	bool UpdateConstantBuffer(const Buffer& aConstantBuffer, const void* aBufferData, size_t aBufferDataSize) const;

	void SetVertexBuffer(const Buffer* aBuffer) const;
	void SetIndexBuffer(const Buffer* aBuffer) const;
	void SetConstantBuffer(const Buffer* aBuffer, unsigned aSlot, PipeLineStages aStages) const; 
	void SetPipelineState (const PipelineStateObject* aPSO);

	void Draw(unsigned aNumVertices) const;
	void DrawIndexed(unsigned aIndexCount, unsigned aIndexOffset) const;

private:

	void SetObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& aObject, std::string_view aName) const;

	Microsoft::WRL::ComPtr<ID3D11Device> myDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> myContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mySwapChain;
	
	HWND myWindowHandle;
};

