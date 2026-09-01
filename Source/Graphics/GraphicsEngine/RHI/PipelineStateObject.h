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
    ShaderData GeometryShader;

    std::vector<VertexElementDesc> InputLayoutElements;
    Topology Topology = Topology::TriangleList;
    RasterizerStateDescription RasterizerState;
    BlendMode BlendMode = BlendMode::Opaque;
};

class PipelineStateObject
{
    friend class RenderHardwareInterface;
    friend class GraphicsCommandList;
    
public:
    PipelineStateObject();
    ~PipelineStateObject();

private:
    
	Microsoft::WRL::ComPtr<struct ID3D11InputLayout> myInputLayout;
	Microsoft::WRL::ComPtr<struct ID3D11VertexShader> myVertexShader;
	Microsoft::WRL::ComPtr<struct ID3D11PixelShader> myPixelShader;
	Microsoft::WRL::ComPtr<struct ID3D11GeometryShader> myGeometryShader;
	Microsoft::WRL::ComPtr<struct ID3D11RasterizerState> myRasterizerState;
    Microsoft::WRL::ComPtr<struct ID3D11BlendState> myBlendState;

    std::string myName;
    Topology myTopology;
    BlendMode myBlendMode;
};
