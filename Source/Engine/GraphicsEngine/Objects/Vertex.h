#pragma once

#include "GraphicsEngine/RHI/RHIStructs.h"

#include <CommonUtilities/Vector.hpp>
#include <vector>

struct Vertex
{
	CommonUtilities::Vector4f Position = { 0, 0, 0, 1 };
	CommonUtilities::Vector4f Color = { 1, 1, 1, 1 };
	CommonUtilities::Vector4u BoneIDs = { 0, 0, 0, 0 };
	CommonUtilities::Vector4f SkinWeights = { 0, 0, 0, 0 };

	CommonUtilities::Vector2f UV0 = { 0, 0 };
	CommonUtilities::Vector2f UV1 = { 0, 0 };

	CommonUtilities::Vector3f Normal = { 0, 0, 0 };
	CommonUtilities::Vector3f Tangent = { 0, 0, 0 };

	static const std::vector<VertexElementDesc> Description;
};

struct SimpleVertex
{
	CommonUtilities::Vector4f Position = { 0, 0, 0, 1 };
	CommonUtilities::Vector4f Color = { 1, 1, 1, 1 };

	static const std::vector<VertexElementDesc> Description;
};
