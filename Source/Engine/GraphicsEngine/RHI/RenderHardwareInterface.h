#pragma once
#include "CommonUtilities/Vector.hpp"

#include <string_view>
#include <vector>
#include <Windows.h>
#include <wrl.h>
#include <wrl/client.h>
#include <filesystem>

class Sampler;
struct SamplerDescription;
class GBuffer;
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

typedef int ResourceBindFlags;

class RenderHardwareInterface
{
public:
    RenderHardwareInterface();
    ~RenderHardwareInterface();

    bool Initialize(HWND aWindowHandle, bool aEnableDebug, Texture& outBackBuffer, Texture& outIntermediateLightBuffer, Texture& outDepthStencil, GBuffer& outGBuffer);

    CommonUtilities::Vector2u GetClientSize() const;
	bool Resize(unsigned aWidth, unsigned aHeight, Texture& outBackBuffer, Texture& outDepthStencil);

    bool CreateVertexBuffer(std::string_view aName, const std::vector<Vertex>& aVertexList, Buffer& outBuffer) const;
    bool CreateDynamicVertexBuffer(std::string_view aName, size_t aMaxVertexCount, Buffer& outBuffer) const;
    bool CreateDynamicVertexBufferSimple(std::string_view aName, size_t aMaxVertexCount, Buffer& outBuffer) const;
    bool CreateIndexBuffer(std::string_view aName, const std::vector<unsigned>& aIndexList, Buffer& outBuffer) const;
    bool CreateConstantBuffer(std::string_view aName, size_t aSize, Buffer& outBuffer) const;

    bool CreateDepthStencil(std::string_view aName, unsigned aWidth, unsigned aHeight, Texture& outDepthStencil, bool aCube) const;
    bool CreatePipelineStateObject(const PipelineStateDescription& aDescription, PipelineStateObject& outPSO) const;
    bool CreateCommandList(std::string_view aName, GraphicsCommandList& outCommandList) const;
    bool CreateTexture(std::string_view aName, const uint8_t* aByteCode, size_t aByteCodeSize, Texture& outTexture) const;
    bool CreateTexture(std::string_view aName, unsigned aWidth, unsigned aHeight, unsigned aNumMips, ResourceFormat aFormat, ResourceBindFlags aBindFlags, Texture& outTexture) const;
    bool CreateColorTexture(std::string_view aName, const std::array<uint8_t, 4>& aColor, Texture& outTexture) const;
    bool CreateColorCubeTexture(std::string_view aName, const std::array<uint8_t, 4>& aColor, Texture& outTexture) const;
    bool CreateSampler(const SamplerDescription& aDescription, Sampler& outSampler) const;
    bool CreateGBuffer(std::string_view aName, unsigned aWidth, unsigned aHeight, GBuffer& outGBuffer) const;

    void ExecuteCommandList(const GraphicsCommandList& aCommandList) const;

    void Present() const;

    bool CompileShader(ShaderType aShaderType, const std::filesystem::path& aPath, ID3DInclude* aIncludeHandler, bool aCompileDebug, Shader& outShader) const;

	ID3D11Device* GetNativeDevice() const;
	ID3D11DeviceContext* GetNativeDeviceContext() const;

private:
    void SetObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& aObject, std::string_view aName) const;

    Microsoft::WRL::ComPtr<ID3D11Device> myDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> myContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mySwapChain;

    HWND myWindowHandle = nullptr;
};

