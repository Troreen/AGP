#include "AnimatorComponent.h"

#include "../Animation/AnimationManager.h"

#include <GameFramework/ServiceLocator.h>
#include <GameFramework/AssetHandling/MeshAsset.h>

AnimatorComponent::AnimatorComponent() : myMeshComponent(nullptr), myAnimationTree(nullptr)
{
	myAnimations;
}

void AnimatorComponent::Init(const std::string& aTree)
{
	if (!myAnimationTree)
	{
		delete myAnimationTree;
	}
	myAnimationTree = new AnimationTree(ServiceLocator::GetInstance().GetAnimationManager().GetAnimationTree(aTree));

	myAnimationTree->SetAnimationPlayer(this);
}

void AnimatorComponent::Update(float aDeltaTime)
{
	if (myAnimationTree)
	{
		myAnimationTree->Update(aDeltaTime);
	}

	const bool baseChanged = AdvancePlayback(myBaseLayer, aDeltaTime);
	const bool partialChanged = AdvancePlayback(myPartialLayer, aDeltaTime);
	if (baseChanged || partialChanged)
	{
		RebuildJointTransforms();
	}
}

std::shared_ptr<AnimationAsset> AnimatorComponent::GetAnimation(const std::string& aName) const
{
	const auto foundAnimation = myAnimations.find(std::string(aName));
	if (foundAnimation == myAnimations.end())
	{
		return nullptr;
	}

	return foundAnimation->second;
}

void AnimatorComponent::SetMeshComponent(SkeletalMeshComponent* aMeshComponent)
{
	myMeshComponent = aMeshComponent;
}

void AnimatorComponent::AddAnimation(const std::string& aName, const std::shared_ptr<AnimationAsset>& anAnimation)
{
	if (anAnimation == nullptr || anAnimation->GetAnimation() == nullptr ||
		anAnimation->GetAnimation()->Name.empty() || !anAnimation->GetAnimation()->IsValid() ||
		myAnimations.find(aName) != myAnimations.end())
	{
		return;
	}

	myAnimations.emplace(aName, anAnimation);
}

const AnimatorComponent::PlaybackState& AnimatorComponent::GetCurrentPlayBackState(bool aIsBaseLayer) const
{
	if (aIsBaseLayer)
	{
		return myBaseLayer;
	}
	else
	{
		return myPartialLayer;
	}
}

bool AnimatorComponent::PlayAnimation(const std::string& anAnimationName, bool aShouldLoop)
{
	if (myMeshComponent->GetMesh() == nullptr)
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

AnimationTree& AnimatorComponent::GetTree()
{
	return *myAnimationTree;
}

bool AnimatorComponent::PlayPartialAnimation(const std::string& anAnimationName, bool aShouldLoop)
{
	if (myMeshComponent->GetMesh() == nullptr)
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

bool AnimatorComponent::ConfigurePartialLayerFromJointName(const std::string& aRootJointName)
{
	myPartialLayerMask.fill(false);

	if (myMeshComponent->GetMesh() == nullptr)
	{
		return false;
	}

	const Skeleton* skeleton = myMeshComponent->GetMesh()->GetMesh()->GetSkeleton();
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

//void AnimatorComponent::OnMeshChanged()
//{
//	myBaseLayer = {};
//	myPartialLayer = {};
//	myPartialLayerMask.fill(false);
//	ResetJointTransforms();
//}
//

bool AnimatorComponent::AdvancePlayback(PlaybackState& aPlayback, float aDeltaTime)
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
			aPlayback.Looped = true;
		}
		else
		{
			aPlayback.Active = false;
			break;
		}
	}

	return advanced;
}

void AnimatorComponent::RebuildJointTransforms()
{
	myMeshComponent->ResetJointTransforms();

	UpdateJointPose(0, CommonUtilities::Matrix4f());
}

void AnimatorComponent::UpdateJointPose(std::size_t aJointIndex, const CommonUtilities::Matrix4f& aParentJointTransform)
{
	const Skeleton* skeleton = myMeshComponent->GetMesh() != nullptr ? myMeshComponent->GetMesh()->GetMesh()->GetSkeleton() : nullptr;
	if (skeleton == nullptr || aJointIndex >= skeleton->Joints.size() || aJointIndex >= myMeshComponent->myJointTransforms.size())
	{
		return;
	}

	const Skeleton::Joint& joint = skeleton->Joints[aJointIndex];
	const CommonUtilities::Matrix4f jointTransform = GetLocalTransformForJoint(aJointIndex) * aParentJointTransform;
	myMeshComponent->myJointTransforms[aJointIndex] = joint.BindPoseInverse * jointTransform;

	for (const int childIndex : joint.Children)
	{
		if (childIndex >= 0)
		{
			UpdateJointPose(static_cast<size_t>(childIndex), jointTransform);
		}
	}
}

void AnimatorComponent::MarkJointAndChildren(std::size_t aJointIndex)
{
	const Skeleton* skeleton = myMeshComponent->GetMesh() != nullptr ? myMeshComponent->GetMesh()->GetMesh()->GetSkeleton() : nullptr;
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

const CommonUtilities::Matrix4f& AnimatorComponent::GetLocalTransformForJoint(std::size_t aJointIndex) const
{
	static const CommonUtilities::Matrix4f identity;

	const Skeleton* skeleton = myMeshComponent->GetMesh() != nullptr ? myMeshComponent->GetMesh()->GetMesh()->GetSkeleton() : nullptr;
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
