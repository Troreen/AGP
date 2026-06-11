#pragma once
#include <wrl/client.h>
#include "GraphicsEngine/RHI/RHIStructs.h"

struct ID3D10Blob;
typedef ID3D10Blob ID3DBlob;

class Shader
{
    friend class RenderHardwareInterface;

public:
    Shader();
    ~Shader();    

    ShaderType GetShaderType() const { return myType; }
    const uint8_t* GetDataPtr() const { return static_cast<uint8_t*>(myBlob->GetBufferPointer()); };
    size_t GetDataSize() const { return myBlob->GetBufferSize(); }
    
private:
    Microsoft::WRL::ComPtr<ID3DBlob> myBlob;
    ShaderType myType = ShaderType::Unknown;
};
