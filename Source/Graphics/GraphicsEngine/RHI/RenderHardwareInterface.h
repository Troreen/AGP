#pragma once
#include <filesystem>
#include <array>
#include <memory>
#include <wrl.h>
#include <Windows.h>
#include <string_view>
#include <vector>

#include "RHIStructs.h"
#include "Vector.hpp"

class Sampler;
struct SamplerDescription;
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

struct ID3DInclude;
class Shader;
class FrameBenchmarkSession;

class RenderHardwareInterface
{
public:	
	enum class ResizeBackBufferResult
	{
		Completed,
		FailedTargetsPreserved,
		FailedTargetsUnavailable
	};

	RenderHardwareInterface();
	~RenderHardwareInterface();

	bool Initialize(HWND aWindowHandle, bool aEnableDebug, Texture& outBackBuffer, Texture& outDepthStencil);

	
	CommonUtilities::Vector2u GetClientSize() const;
	
	bool CreateVertexBuffer(std::string_view aName, const std::vector<Vertex>&  aVertexList, Buffer& outBuffer) const;
	bool CreateIndexBuffer(std::string_view aName, const std::vector<unsigned>&  aIndexList, Buffer& outBuffer) const;
	bool CreateConstantBuffer(std::string_view aName, size_t aSize, Buffer& outBuffer) const;
	
	bool CreatePipelineStateObject (const PipelineStateDescription& aDescription, PipelineStateObject& outPSO) const;
	bool CreateCommandList(std::string_view aName, GraphicsCommandList& outCommandList) const;
	bool CreateDepthStencil(std::string_view aName, unsigned aWidth, unsigned aHeight, Texture& outDepthStencil, bool aCubeMap = false) const;
	bool CreateRenderTargetTexture(std::string_view aName, unsigned aWidth, unsigned aHeight, unsigned aFormat, Texture& outTexture) const;
	bool CreateTexture(std::string_view aName, const uint8_t* aByteCode, size_t aByteCodeSize, Texture& outTexture) const;
	bool CreateColorTexture(std::string_view aName, const std::array<uint8_t, 4>& aColor, Texture& outTexture) const;
	bool CreateSampler(const SamplerDescription& aDescription, Sampler& outSampler) const; 

	void ExecuteCommandList(const GraphicsCommandList& aCommandList) const;
	bool BeginBackBufferFrame(const Texture& aBackBuffer, const Texture& aDepthStencil, const std::array<float, 4>& aClearColor) const;
	ResizeBackBufferResult ResizeBackBuffer(unsigned aWidth, unsigned aHeight, Texture& outBackBuffer, Texture& outDepthStencil);
	ID3D11Device* GetNativeDevice() const { return myDevice.Get(); }
	ID3D11DeviceContext* GetNativeImmediateContext() const { return myContext.Get(); }
	
	bool Present() const;
	
	bool CompileShader(ShaderType aShaderType, const std::filesystem::path& aPath, ID3DInclude* aIncludeHandler, bool aCompileDebug, Shader& outShader) const;

private:

	void SetObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& aObject, std::string_view aName) const;

	Microsoft::WRL::ComPtr<ID3D11Device> myDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> myContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mySwapChain;
	mutable std::unique_ptr<FrameBenchmarkSession> myFrameBenchmark;
	
	HWND myWindowHandle;
};

