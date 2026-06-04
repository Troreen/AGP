#pragma once
#include <cstddef>
#include <string>
#include <wrl/client.h>
#include "GraphicsEngine/RHI/RHIStructs.h"

struct ID3D11Buffer;

class Buffer
{
    friend class RenderHardwareInterface;
    
public:
    Buffer();
    ~Buffer();

private:

    std::string myName;
    Microsoft::WRL::ComPtr<ID3D11Buffer> myBuffer;
    size_t mySize;
    unsigned myStride;
    BufferType myType;
};

