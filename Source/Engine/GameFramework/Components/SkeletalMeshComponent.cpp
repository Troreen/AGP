#include "GameFramework/Components/SkeletalMeshComponent.h"

#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GraphicsEngine/Objects/Mesh.h"

#include <utility>

SkeletalMeshComponent::SkeletalMeshComponent()
{
	OnMeshChanged();
}

SkeletalMeshComponent::SkeletalMeshComponent(const std::shared_ptr<MeshAsset>& aMesh) : MeshComponentBase(aMesh)
{
	OnMeshChanged();
}

void SkeletalMeshComponent::Update(float aDeltaTime)
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

bool SkeletalMeshComponent::HasSkinning() const
{
	return myMesh != nullptr && myMesh->GetMesh()->HasSkeleton() && myBaseLayer.Active;
}

const std::array<CommonUtilities::Matrix4f, 128>* SkeletalMeshComponent::GetJointTransforms() const
{
	return &myJointTransforms;
}

void SkeletalMeshComponent::AddAnimation(std::string_view aName, const std::shared_ptr<AnimationAsset>& anAnimation)
{
	if (anAnimation == nullptr || anAnimation->GetAnimation() == nullptr ||
		anAnimation->GetAnimation()->Name.empty() || !anAnimation->GetAnimation()->IsValid())
	{
		return;
	}

	myAnimations[aName.data()] = anAnimation;
}

std::shared_ptr<AnimationAsset> SkeletalMeshComponent::GetAnimation(std::string_view aName) const
{
	const auto foundAnimation = myAnimations.find(std::string(aName));
	if (foundAnimation == myAnimations.end())
	{
		return nullptr;
	}

	return foundAnimation->second;
}

bool SkeletalMeshComponent::PlayAnimation(std::string_view anAnimationName, bool aShouldLoop)
{
	if (myMesh == nullptr)
	{
		return false;
	}

	const std::shared_ptr<AnimationAsset>& animation = GetAnimation(anAnimationName);
	if (animation == nullptr)
	{
		return false;
	}

	if (myBaseLayer.Active && myBaseLayer.AnimationName == anAnimationName)
	{
		myBaseLayer.Looping = aShouldLoop;
		return true;
	}

	myBaseLayer.CurrentAnimation = animation;
	myBaseLayer.AnimationName = std::string(anAnimationName);
	myBaseLayer.CurrentFrame = 0;
	myBaseLayer.Timer = 0.0f;
	myBaseLayer.Looping = aShouldLoop;
	myBaseLayer.Active = true;
	RebuildJointTransforms();
	return true;
}

bool SkeletalMeshComponent::PlayPartialAnimation(std::string_view anAnimationName, bool aShouldLoop)
{
	if (myMesh == nullptr)
	{
		return false;
	}

	const std::shared_ptr<AnimationAsset>& animation = GetAnimation(anAnimationName);
	if (animation == nullptr)
	{
		return false;
	}

	if (myPartialLayer.Active && myPartialLayer.AnimationName == anAnimationName)
	{
		myPartialLayer.Looping = aShouldLoop;
		return true;
	}

	myPartialLayer.CurrentAnimation = animation;
	myPartialLayer.AnimationName = std::string(anAnimationName);
	myPartialLayer.CurrentFrame = 0;
	myPartialLayer.Timer = 0.0f;
	myPartialLayer.Looping = aShouldLoop;
	myPartialLayer.Active = true;
	RebuildJointTransforms();
	return true;
}

bool SkeletalMeshComponent::ConfigurePartialLayerFromJointName(std::string_view aRootJointName)
{
	myPartialLayerMask.fill(false);

	if (myMesh == nullptr)
	{
		return false;
	}

	const Skeleton* skeleton = myMesh->GetMesh()->GetSkeleton();
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

void SkeletalMeshComponent::OnMeshChanged()
{
	myBaseLayer = {};
	myPartialLayer = {};
	myPartialLayerMask.fill(false);
	ResetJointTransforms();
}

void SkeletalMeshComponent::ResetJointTransforms()
{
	for (CommonUtilities::Matrix4f& transform : myJointTransforms)
	{
		transform = CommonUtilities::Matrix4f();
	}
}

bool SkeletalMeshComponent::AdvancePlayback(PlaybackState& aPlayback, float aDeltaTime)
{
	if (!aPlayback.Active || aPlayback.CurrentAnimation == nullptr || !aPlayback.CurrentAnimation->GetAnimation()->IsValid())
	{
		return false;
	}

	const float frameTime = 1.0f / aPlayback.CurrentAnimation->GetAnimation()->FramesPerSecond;
	aPlayback.Timer += aDeltaTime;

	bool advanced = false;
	while (aPlayback.Timer >= frameTime)
	{
		aPlayback.Timer -= frameTime;
		advanced = true;

		if (aPlayback.CurrentFrame + 1 < aPlayback.CurrentAnimation->GetAnimation()->Frames.size())
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

void SkeletalMeshComponent::RebuildJointTransforms()
{
	ResetJointTransforms();

	if (!HasSkinning())
	{
		return;
	}

	UpdateJointPose(0, CommonUtilities::Matrix4f());
}

void SkeletalMeshComponent::UpdateJointPose(std::size_t aJointIndex, const CommonUtilities::Matrix4f& aParentJointTransform)
{
	const Skeleton* skeleton = myMesh != nullptr ? myMesh->GetMesh()->GetSkeleton() : nullptr;
	if (skeleton == nullptr || aJointIndex >= skeleton->Joints.size() || aJointIndex >= myJointTransforms.size())
	{
		return;
	}

	const Skeleton::Joint& joint = skeleton->Joints[aJointIndex];
	const CommonUtilities::Matrix4f jointTransform = GetLocalTransformForJoint(aJointIndex) * aParentJointTransform;
	myJointTransforms[aJointIndex] = joint.BindPoseInverse * jointTransform;

	for (const int childIndex : joint.Children)
	{
		if (childIndex >= 0)
		{
			UpdateJointPose(static_cast<size_t>(childIndex), jointTransform);
		}
	}
}

const CommonUtilities::Matrix4f& SkeletalMeshComponent::GetLocalTransformForJoint(std::size_t aJointIndex) const
{
	static const CommonUtilities::Matrix4f identity;

	const Skeleton* skeleton = myMesh != nullptr ? myMesh->GetMesh()->GetSkeleton() : nullptr;
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

	if (selectedLayer->CurrentAnimation == nullptr || selectedLayer->CurrentFrame >= selectedLayer->CurrentAnimation->GetAnimation()->Frames.size())
	{
		return identity;
	}

	const Animation::Frame& selectedFrame = selectedLayer->CurrentAnimation->GetAnimation()->Frames[selectedLayer->CurrentFrame];
	const auto selectedTransform = selectedFrame.Transforms.find(jointName);
	if (selectedTransform != selectedFrame.Transforms.end())
	{
		return selectedTransform->second;
	}

	if (selectedLayer == &myPartialLayer && myBaseLayer.CurrentAnimation != nullptr &&
	    myBaseLayer.CurrentFrame < myBaseLayer.CurrentAnimation->GetAnimation()->Frames.size())
	{
		const Animation::Frame& baseFrame = myBaseLayer.CurrentAnimation->GetAnimation()->Frames[myBaseLayer.CurrentFrame];
		const auto baseTransform = baseFrame.Transforms.find(jointName);
		if (baseTransform != baseFrame.Transforms.end())
		{
			return baseTransform->second;
		}
	}

	return identity;
}

void SkeletalMeshComponent::MarkJointAndChildren(std::size_t aJointIndex)
{
	const Skeleton* skeleton = myMesh != nullptr ? myMesh->GetMesh()->GetSkeleton() : nullptr;
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
