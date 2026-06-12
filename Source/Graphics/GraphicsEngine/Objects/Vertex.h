#pragma once 

#include "Vector.hpp"
#include "GraphicsEngine/RHI/RHIStructs.h"

struct Vertex
{
    CommonUtilities::Vector4f Position = { 0, 0, 0, 1 };
    CommonUtilities::Vector4f Color = { 1, 1, 1, 1 };
    CommonUtilities::Vector4u BoneIDs = { 0, 0, 0, 0 };
    CommonUtilities::Vector4f SkinWeights = { 0, 0, 0, 0 };

	CommonUtilities::Vector2f UV0 = { 0, 0 };
    CommonUtilities::Vector2f UV1 = { 0, 0 };
	CommonUtilities::Vector3f Normal = { 0, 0, 1 };
	CommonUtilities::Vector3f Tangent = { 1, 0, 0 };


	static const std::vector<VertexElementDesc> Description;
};
