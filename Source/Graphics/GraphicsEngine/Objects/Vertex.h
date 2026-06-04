#pragma once 

#include "Vector.hpp"
#include "GraphicsEngine/RHI/RHIStructs.h"

struct Vertex
{
    CommonUtilities::Vector4f Position = { 0, 0, 0, 1 };
    CommonUtilities::Vector4f Color = { 1, 1, 1, 1 };

	static const std::vector<VertexElementDesc> Description;
};