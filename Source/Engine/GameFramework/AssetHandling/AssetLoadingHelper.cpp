#include "AssetLoadingHelper.h"

CommonUtilities::Matrix4f AssetHelper::ConvertMatrix(const TGA::FBX::Matrix& aSourceMatrix)
{
	return {
		aSourceMatrix(1, 1), aSourceMatrix(1, 2), aSourceMatrix(1, 3), aSourceMatrix(1, 4),
		aSourceMatrix(2, 1), aSourceMatrix(2, 2), aSourceMatrix(2, 3), aSourceMatrix(2, 4),
		aSourceMatrix(3, 1), aSourceMatrix(3, 2), aSourceMatrix(3, 3), aSourceMatrix(3, 4),
		aSourceMatrix(4, 1), aSourceMatrix(4, 2), aSourceMatrix(4, 3), aSourceMatrix(4, 4)
	};
}

std::shared_ptr<Animation> AssetHelper::ConvertAnimation(const TGA::FBX::Animation& aSourceAnimation)
{
	std::shared_ptr<Animation> animation = std::make_shared<Animation>();
	animation->Name = aSourceAnimation.Name;
	animation->Duration = static_cast<float>(aSourceAnimation.Duration);
	animation->FramesPerSecond = aSourceAnimation.FramesPerSecond;
	animation->Frames.reserve(aSourceAnimation.Frames.size());

	for (const TGA::FBX::Animation::Frame& sourceFrame : aSourceAnimation.Frames)
	{
		Animation::Frame frame;
		frame.Transforms.reserve(sourceFrame.LocalTransforms.size());
		for (const auto& [jointName, sourceTransform] : sourceFrame.LocalTransforms)
		{
			frame.Transforms.emplace(jointName, ConvertMatrix(sourceTransform));
		}
		animation->Frames.push_back(std::move(frame));
	}

	return animation;
}

bool AssetHelper::HasImportedColor(const TGA::FBX::Vertex& aVertex)
{
	return aVertex.VertexColors[0][3] > 0.0f;
}

CU::Vector3f AssetHelper::ConvertDirection(const float* aSourceVector, const CU::Vector3f& aFallback)
{
	CommonUtilities::Vector3f direction(
		aSourceVector[0],
		aSourceVector[1],
		aSourceVector[2]);

	if (direction.LengthSqr() <= 0.000001f)
	{
		return aFallback;
	}

	return direction.GetNormalized();
}

Vertex AssetHelper::ConvertVertex(const TGA::FBX::Vertex& aSourceVertex)
{
	const CommonUtilities::Vector4f DefaultVertexColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	Vertex vertex;
	vertex.Position = {
		aSourceVertex.Position[0],
		aSourceVertex.Position[1],
		aSourceVertex.Position[2],
		aSourceVertex.Position[3]
	};

	if (HasImportedColor(aSourceVertex))
	{
		vertex.Color = {
			aSourceVertex.VertexColors[0][0],
			aSourceVertex.VertexColors[0][1],
			aSourceVertex.VertexColors[0][2],
			aSourceVertex.VertexColors[0][3]
		};
	}
	else
	{
		vertex.Color = DefaultVertexColor;
	}

	vertex.BoneIDs = {
		aSourceVertex.BoneIDs[0],
		aSourceVertex.BoneIDs[1],
		aSourceVertex.BoneIDs[2],
		aSourceVertex.BoneIDs[3]
	};

	vertex.SkinWeights = {
		aSourceVertex.BoneWeights[0],
		aSourceVertex.BoneWeights[1],
		aSourceVertex.BoneWeights[2],
		aSourceVertex.BoneWeights[3]
	};

	vertex.UV0 = {
		aSourceVertex.UVs[0][0],
		aSourceVertex.UVs[0][1]
	};

	vertex.UV1 = {
		aSourceVertex.UVs[1][0],
		aSourceVertex.UVs[1][1]
	};

	vertex.Normal = ConvertDirection(aSourceVertex.Normal, CommonUtilities::Vector3f::UnitZ);
	vertex.Tangent = ConvertDirection(aSourceVertex.Tangent, CommonUtilities::Vector3f::UnitX);

	const float totalWeight =
		vertex.SkinWeights.x +
		vertex.SkinWeights.y +
		vertex.SkinWeights.z +
		vertex.SkinWeights.w;
	if (totalWeight > 0.0f)
	{
		vertex.SkinWeights.x /= totalWeight;
		vertex.SkinWeights.y /= totalWeight;
		vertex.SkinWeights.z /= totalWeight;
		vertex.SkinWeights.w /= totalWeight;
	}

	return vertex;
}

Skeleton AssetHelper::ConvertSkeleton(const TGA::FBX::Skeleton& aSourceSkeleton)
{
	Skeleton skeleton;
	skeleton.Joints.reserve(aSourceSkeleton.Bones.size());

	for (const TGA::FBX::Skeleton::Bone& sourceBone : aSourceSkeleton.Bones)
	{
		Skeleton::Joint joint;
		joint.BindPoseInverse = ConvertMatrix(sourceBone.BindPoseInverse).GetTranspose();
		joint.Parent = sourceBone.ParentIdx;
		joint.Name = sourceBone.Name;
		joint.Children.reserve(sourceBone.Children.size());

		for (const unsigned childIndex : sourceBone.Children)
		{
			joint.Children.push_back(static_cast<int>(childIndex));
		}

		skeleton.Joints.push_back(std::move(joint));
	}

	for (size_t jointIndex = 0; jointIndex < skeleton.Joints.size(); ++jointIndex)
	{
		skeleton.JointNameToIndex[skeleton.Joints[jointIndex].Name] = jointIndex;
	}

	return skeleton;
}
