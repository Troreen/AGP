#pragma once

#include "RHIStructs.h"

#include <string>
#include <vector>
#include <wrl.h>

struct PipelineStateDescription
{
	std::string Name;

	struct ShaderData
	{
		const uint8_t* ByteCode = nullptr;
		size_t ByteCodeSize = 0;
	};

	ShaderData VertexShader;
	ShaderData GeometryShader;
	ShaderData PixelShader;

	std::vector<VertexElementDesc> InputLayoutElements;
	Topology Topology = Topology::TriangleList;

	RasterizerState FillMode = RasterizerState::FillSolid;
	RasterizerCullMode CullMode = RasterizerCullMode::Back;
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
};
