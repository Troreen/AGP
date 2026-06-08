#pragma once
#include <wrl.h>
#include <Windows.h>
#include <string_view>
#include <vector>

#include "RHIStructs.h"
#include "Vector.hpp"

class GraphicsCommandList;
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

	
	CommonUtilities::Vector2u GetClientSize() const;
	
	bool CreateVertexBuffer(std::string_view aName, const std::vector<Vertex>&  aVertexList, Buffer& outBuffer) const;
	bool CreateIndexBuffer(std::string_view aName, const std::vector<unsigned>&  aIndexList, Buffer& outBuffer) const;
	bool CreateConstantBuffer(std::string_view aName, size_t aSize, Buffer& outBuffer) const;
	
	bool CreatePipelineStateObject (const PipelineStateDescription& aDescription, PipelineStateObject& outPSO) const;
	bool CreateCommandList(std::string_view aName, GraphicsCommandList& outCommandList) const;
	
	void ExecuteCommandList(const GraphicsCommandList& aCommandList) const;
	
	void Present() const; 
	
private:

	void SetObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& aObject, std::string_view aName) const;

	Microsoft::WRL::ComPtr<ID3D11Device> myDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> myContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mySwapChain;
	
	HWND myWindowHandle;
};

