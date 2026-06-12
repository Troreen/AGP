#pragma once
#include <wrl.h>
#include "GraphicsEngine/RHI/RHIStructs.h"

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Resource;
struct ID3D11ShaderResourceView;

class Texture
{
    friend class RenderHardwareInterface;
    friend class GraphicsCommandList;

public:
    Texture();
    ~Texture();
    
private:

	std::string myName;
	Microsoft::WRL::ComPtr<ID3D11Resource> myResource;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> myRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> myDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mySRV;
    Viewport myViewport;
};
