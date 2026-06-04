#include "GraphicsEngine.pch.h"
#include "Vertex.h"

const std::vector<VertexElementDesc> Vertex::Description =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT }
};