#pragma once

#include "GraphicsEngine/RHI/RHIStructs.h"

#include <string>
#include <wrl/client.h>

struct ID3D11Buffer;

class Buffer
{
    friend class RenderHardwareInterface;
    friend class GraphicsCommandList;

public:
    Buffer();
    ~Buffer();

    bool IsValid() const { return myBuffer && myType != BufferType::Unknown; }

private:
    std::string myName;
    Microsoft::WRL::ComPtr<ID3D11Buffer> myBuffer;
    size_t mySize;
    unsigned myStride;
    BufferType myType;
};
