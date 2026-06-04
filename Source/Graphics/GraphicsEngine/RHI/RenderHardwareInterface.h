#pragma once
#include <wrl.h>
#include <Windows.h>
#include <string_view>
#include <vector>

#include "Vector.hpp"


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

	bool Initialize(HWND aWindowHandle, bool aEnableDebug, Texture& outBackBuffer);

	void Present() const; 
	void ClearRenderTarget(const Texture& aTarget) const;
	void SetRenderTarget(const Texture* aTarget) const;

	CommonUtilities::Vector2u GetClientSize() const;

	bool CreateVertexBuffer(std::string_view aName, const std::vector<Vertex>&  aVertexList, Buffer& outBuffer) const;
	void SetVertexBuffer(const Buffer* aBuffer) const;

	void Draw(unsigned aNumVertices) const;

private:

	void SetObjectName(const Microsoft::WRL::ComPtr<ID3D11DeviceChild>& aObject, std::string_view aName) const;

	Microsoft::WRL::ComPtr<ID3D11Device> myDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> myContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mySwapChain;
	
	HWND myWindowHandle;

	//TEMP
	Microsoft::WRL::ComPtr<struct ID3D11InputLayout> myTempIL;
	Microsoft::WRL::ComPtr<struct ID3D11VertexShader> myTempVS;
	Microsoft::WRL::ComPtr<struct ID3D11PixelShader> myTempPS;

};

