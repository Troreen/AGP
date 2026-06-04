#pragma once
#include <wrl.h>
#include "GraphicsEngine/RHI/RHIStructs.h"

struct ID3D11RenderTargetView;

class Texture
{
    friend class RenderHardwareInterface;

public:
    Texture();
    ~Texture();
    
private:

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> myRTV;
	Viewport myViewport;
};