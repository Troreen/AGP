#pragma once
#include <cstdint>
#include <string>

struct Viewport
{
	float TopLeftX = 0;
	float TopLeftY = 0;
	float Width = 0;
	float Height = 0;
	float MinDepth = 0;
	float MaxDepth = 1;
};

enum class BufferType : uint8_t
{
	Unknown,
	VertexBuffer,
	IndexBuffer,
	ConstantBuffer
};

enum class Topology : unsigned
{
	Undefined = 0,
	TriangleList = 4
};

enum class RasterizerFillMode : unsigned
{
	Wireframe = 2,
	Solid = 3
};

enum class RasterizerCullMode : unsigned
{
	None = 1,
	Front = 2,
	Back = 3
};

struct RasterizerStateDescription
{
	RasterizerFillMode FillMode = RasterizerFillMode::Solid;
	RasterizerCullMode CullMode = RasterizerCullMode::Back;
	int DepthBias = 0;
	float DepthBiasClamp = 0.0f;
	float SlopeScaledDepthBias = 0.0f;
	bool DepthClipEnable = true;
};

struct VertexElementDesc
{
	std::string Semantic;
	unsigned SemanticIndex = 0;
	unsigned Format = 0;
};

enum PipeLineStage_
{
	PipeLineStage_None = 0,
	PipeLineStage_InputAssembler = 1,
	PipeLineStage_VertexShader = 2,
	PipeLineStage_GeometryShader = 4,
	PipeLineStage_ComputeShader = 8,
	PipeLineStage_Rasterizer = 16,
	PipeLineStage_PixelShader = 32,
	PipeLineStage_OutputMerger = 64,
};

typedef int PipeLineStages;

enum class ShaderType : unsigned
{
	Unknown,
	VertexShader,
	PixelShader,
	GeometryShader,
	ComputeShader
};
