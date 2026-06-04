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
	VertexBuffer
};

enum class Topology : unsigned
{
	Undefined = 0,
	TriangleList = 4
};

struct VertexElementDesc
{
	std::string Semantic;
	unsigned SemanticIndex = 0;
	unsigned Format = 0;
};
