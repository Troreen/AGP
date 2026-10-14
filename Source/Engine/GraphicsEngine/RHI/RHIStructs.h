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

enum class BlendMode : uint8_t
{
	Opaque,
	Alpha,
	Additive,
	Bitmasking,
	AlphaToCoverage
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
	TriangleList = 4,
	TriangleStrip = 5
};

struct VertexElementDesc
{
	std::string Semantic;
	unsigned SemanticIndex = 0;
	unsigned Format = 0;
};

enum PipelineStage_
{
	PipelineStage_None = 0,
	PipelineStage_InputAssembler = 1,
	PipelineStage_VertexShader = 2,
	PipelineStage_GeometryShader = 4,
	PipelineStage_ComputeShader = 8,
	PipelineStage_Rasterizer = 16,
	PipelineStage_PixelShader = 32,
	PipelineStage_OutputMerger = 64
};

enum class RasterizerState : uint8_t
{
	FillWire = 2,
	FillSolid = 3
};

enum class RasterizerCullMode : uint8_t
{
	None = 1,
	Front = 2,
	Back = 3
};

typedef int PipelineStages;

enum class ShaderType : unsigned
{
	Unknown,
	VertexShader,
	PixelShader,
	GeometryShader,
	ComputeShader
};

enum class ResourceFormat
{
	UNKNOWN = 0,
	R16G16B16A16_FLOAT = 10,
	R32G32_FLOAT = 16,
	R32G8X24_TYPELESS = 19,
	R8G8B8A8_UNORM = 28,
	R16G16_FLOAT = 34,
	R32_FLOAT = 41,
	R8G8_UNORM = 49,
	R16_FLOAT = 54,
	R8_UNORM = 61
};

enum ResourceBindFlag_
{
	ResourceBindFlag_Staging = 0,
	ResourceBindFlag_ShaderResource = 8,
	ResourceBindFlag_RenderTarget = 32,
	ResourceBindFlag_DepthStencil = 64,
	ResourceBindFlag_UnorderedAccessView = 128,
};
