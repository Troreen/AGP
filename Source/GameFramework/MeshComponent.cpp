#include "MeshComponent.h"

#include "GraphicsEngine/Objects/Mesh.h"

#include <utility>

MeshComponent::MeshComponent(std::shared_ptr<Mesh> aMesh)
	: myMesh(std::move(aMesh))
{
	ResetJointTransforms();
}

void MeshComponent::Update(float aDeltaTime)
{
	if (!HasSkinning())
	{
		return;
	}

	const bool baseChanged = AdvancePlayback(myBaseLayer, aDeltaTime);
	const bool partialChanged = AdvancePlayback(myPartialLayer, aDeltaTime);
	if (baseChanged || partialChanged)
	{
		RebuildJointTransforms();
	}
}

void MeshComponent::SetMesh(std::shared_ptr<Mesh> aMesh)
{
	myMesh = std::move(aMesh);
	myBaseLayer = {};
	myPartialLayer = {};
	myPartialLayerMask.fill(false);
	ResetJointTransforms();
}

std::shared_ptr<Mesh> MeshComponent::GetMesh() const
{
	return myMesh;
}

bool MeshComponent::HasMesh() const
{
	return myMesh != nullptr;
}

bool MeshComponent::HasSkinning() const
{
	return myMesh != nullptr && myMesh->HasSkeleton() && myBaseLayer.Active;
}

void MeshComponent::SetVisible(bool aVisible)
{
	SetEnabled(aVisible);
}

bool MeshComponent::IsVisible() const
{
	return IsEnabled();
}

bool MeshComponent::PlayAnimation(std::string_view anAnimationName, bool aShouldLoop)
{
	if (myMesh == nullptr)
	{
		return false;
	}

	std::shared_ptr<Animation> animation = myMesh->GetAnimation(anAnimationName);
	if (animation == nullptr)
	{
		return false;
	}

	if (myBaseLayer.Active && myBaseLayer.AnimationName == anAnimationName)
	{
		myBaseLayer.Looping = aShouldLoop;
		return true;
	}

	myBaseLayer.CurrentAnimation = std::move(animation);
	myBaseLayer.AnimationName = std::string(anAnimationName);
	myBaseLayer.CurrentFrame = 0;
	myBaseLayer.Timer = 0.0f;
	myBaseLayer.Looping = aShouldLoop;
	myBaseLayer.Active = true;
	RebuildJointTransforms();
	return true;
}

bool MeshComponent::PlayPartialAnimation(std::string_view anAnimationName, bool aShouldLoop)
{
	if (myMesh == nullptr)
	{
		return false;
	}

	std::shared_ptr<Animation> animation = myMesh->GetAnimation(anAnimationName);
	if (animation == nullptr)
	{
		return false;
	}

	if (myPartialLayer.Active && myPartialLayer.AnimationName == anAnimationName)
	{
		myPartialLayer.Looping = aShouldLoop;
		return true;
	}

	myPartialLayer.CurrentAnimation = std::move(animation);
	myPartialLayer.AnimationName = std::string(anAnimationName);
	myPartialLayer.CurrentFrame = 0;
	myPartialLayer.Timer = 0.0f;
	myPartialLayer.Looping = aShouldLoop;
	myPartialLayer.Active = true;
	RebuildJointTransforms();
	return true;
}

bool MeshComponent::ConfigurePartialLayerFromJointName(std::string_view aRootJointName)
{
	myPartialLayerMask.fill(false);

	if (myMesh == nullptr)
	{
		return false;
	}

	const Skeleton* skeleton = myMesh->GetSkeleton();
	if (skeleton == nullptr)
	{
		return false;
	}

	const auto rootJoint = skeleton->JointNameToIndex.find(std::string(aRootJointName));
	if (rootJoint == skeleton->JointNameToIndex.end())
	{
		return false;
	}

	MarkJointAndChildren(rootJoint->second);
	return true;
}

const std::array<CU::Matrix4f, 128>& MeshComponent::GetJointTransforms() const
{
	return myJointTransforms;
}

void MeshComponent::ResetJointTransforms()
{
	for (CU::Matrix4f& transform : myJointTransforms)
	{
		transform = CU::Matrix4f();
	}
}

bool MeshComponent::AdvancePlayback(PlaybackState& aPlayback, float aDeltaTime)
{
	if (!aPlayback.Active || aPlayback.CurrentAnimation == nullptr || !aPlayback.CurrentAnimation->IsValid())
	{
		return false;
	}

	const float frameTime = 1.0f / aPlayback.CurrentAnimation->FramesPerSecond;
	aPlayback.Timer += aDeltaTime;

	bool advanced = false;
	while (aPlayback.Timer >= frameTime)
	{
		aPlayback.Timer -= frameTime;
		advanced = true;

		if (aPlayback.CurrentFrame + 1 < aPlayback.CurrentAnimation->Frames.size())
		{
			++aPlayback.CurrentFrame;
			continue;
		}

		if (aPlayback.Looping)
		{
			aPlayback.CurrentFrame = 0;
		}
		else
		{
			aPlayback.Active = false;
			break;
		}
	}

	return advanced;
}

void MeshComponent::RebuildJointTransforms()
{
	ResetJointTransforms();

	if (!HasSkinning())
	{
		return;
	}

	UpdateJointPose(0, CU::Matrix4f());
}

void MeshComponent::UpdateJointPose(size_t aJointIndex, const CU::Matrix4f& aParentJointTransform)
{
	const Skeleton* skeleton = myMesh != nullptr ? myMesh->GetSkeleton() : nullptr;
	if (skeleton == nullptr || aJointIndex >= skeleton->Joints.size() || aJointIndex >= myJointTransforms.size())
	{
		return;
	}

	const Skeleton::Joint& joint = skeleton->Joints[aJointIndex];
	const CU::Matrix4f jointTransform = GetLocalTransformForJoint(aJointIndex) * aParentJointTransform;
	myJointTransforms[aJointIndex] = joint.BindPoseInverse * jointTransform;

	for (const int childIndex : joint.Children)
	{
		if (childIndex >= 0)
		{
			UpdateJointPose(static_cast<size_t>(childIndex), jointTransform);
		}
	}
}

const CU::Matrix4f& MeshComponent::GetLocalTransformForJoint(size_t aJointIndex) const
{
	static const CU::Matrix4f identity;

	const Skeleton* skeleton = myMesh != nullptr ? myMesh->GetSkeleton() : nullptr;
	if (skeleton == nullptr || aJointIndex >= skeleton->Joints.size())
	{
		return identity;
	}

	const std::string& jointName = skeleton->Joints[aJointIndex].Name;

	const PlaybackState* selectedLayer = &myBaseLayer;
	if (myPartialLayer.Active && aJointIndex < myPartialLayerMask.size() && myPartialLayerMask[aJointIndex])
	{
		selectedLayer = &myPartialLayer;
	}

	if (selectedLayer->CurrentAnimation == nullptr || selectedLayer->CurrentFrame >= selectedLayer->CurrentAnimation->Frames.size())
	{
		return identity;
	}

	const Animation::Frame& selectedFrame = selectedLayer->CurrentAnimation->Frames[selectedLayer->CurrentFrame];
	const auto selectedTransform = selectedFrame.Transforms.find(jointName);
	if (selectedTransform != selectedFrame.Transforms.end())
	{
		return selectedTransform->second;
	}

	if (selectedLayer == &myPartialLayer && myBaseLayer.CurrentAnimation != nullptr && myBaseLayer.CurrentFrame < myBaseLayer.CurrentAnimation->Frames.size())
	{
		const Animation::Frame& baseFrame = myBaseLayer.CurrentAnimation->Frames[myBaseLayer.CurrentFrame];
		const auto baseTransform = baseFrame.Transforms.find(jointName);
		if (baseTransform != baseFrame.Transforms.end())
		{
			return baseTransform->second;
		}
	}

	return identity;
}

void MeshComponent::MarkJointAndChildren(size_t aJointIndex)
{
	const Skeleton* skeleton = myMesh != nullptr ? myMesh->GetSkeleton() : nullptr;
	if (skeleton == nullptr || aJointIndex >= skeleton->Joints.size() || aJointIndex >= myPartialLayerMask.size())
	{
		return;
	}

	myPartialLayerMask[aJointIndex] = true;
	for (const int childIndex : skeleton->Joints[aJointIndex].Children)
	{
		if (childIndex >= 0)
		{
			MarkJointAndChildren(static_cast<size_t>(childIndex));
		}
	}
}
