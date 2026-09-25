#pragma once

#include <GraphicsEngine/Objects/Mesh.h>
#include <GraphicsEngine/Objects/Vertex.h>
#include <Importer.h>

namespace AssetHelper
{
	CommonUtilities::Matrix4f ConvertMatrix(const TGA::FBX::Matrix& aSourceMatrix);
	std::shared_ptr<Animation> ConvertAnimation(const TGA::FBX::Animation& aSourceAnimation);
	bool HasImportedColor(const TGA::FBX::Vertex& aVertex);
	CU::Vector3f ConvertDirection(const float* aSourceVector, const CU::Vector3f& aFallback);
	Vertex ConvertVertex(const TGA::FBX::Vertex& aSourceVertex);
	Skeleton ConvertSkeleton(const TGA::FBX::Skeleton& aSourceSkeleton);
}
