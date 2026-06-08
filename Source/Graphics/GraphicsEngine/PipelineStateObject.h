#pragma once
#include "GraphicsEngine/RHI/RHIStructs.h"
#include "wrl.h"
#include <vector>

struct PipelineStateDescription
{
    std::string Name;

    struct ShaderData
    {
        const uint8_t* ByteCode = nullptr;
        size_t ByteCodeSize = 0;
    };

    ShaderData VertexShader;
    ShaderData PixelShader;

    std::vector<VertexElementDesc> InputLayoutElements;
    Topology Topology = Topology::TriangleList;
};

class PipelineStateObject
{
    friend class RenderHardwareInterface;
    
public:
    PipelineStateObject();
    ~PipelineStateObject();

private:
    
	Microsoft::WRL::ComPtr<struct ID3D11InputLayout> myInputLayout;
	Microsoft::WRL::ComPtr<struct ID3D11VertexShader> myVertexShader;
	Microsoft::WRL::ComPtr<struct ID3D11PixelShader> myPixelShader;

    std::string myName;
    Topology myTopology;
};

