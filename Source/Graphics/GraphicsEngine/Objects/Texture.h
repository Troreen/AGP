#pragma once
#include <wrl.h>
#include "GraphicsEngine/RHI/RHIStructs.h"

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

class Texture
{
    friend class RenderHardwareInterface;
    friend class GraphicsCommandList;

public:
    Texture();
    ~Texture();
    
private:

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> myRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> myDSV;
	Viewport myViewport;
};