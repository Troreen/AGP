#pragma once
#include "../Animation/AnimationTree.h"

#include <GameFramework/World/Component.h>
#include <GameFramework/Components/SkeletalMeshComponent.h>


class AnimatorComponent : public Component
{
public:
	AnimatorComponent();

	struct PlaybackState;

	void Init(const std::string& aTree);
	void Update(float aDeltaTime) override;

	void SetMeshComponent(SkeletalMeshComponent* aMeshComponent);
	void AddAnimation(const std::string& aName, const std::shared_ptr<AnimationAsset>& anAnimation);
	std::shared_ptr<AnimationAsset> GetAnimation(const std::string& aName) const;
	const PlaybackState& GetCurrentPlayBackState(bool aIsBaseLayer) const;

	AnimationTree& GetTree();

	bool PlayAnimation(const std::string& anAnimationName, bool aShouldLoop);
	bool PlayPartialAnimation(const std::string& anAnimationName, bool aShouldLoop);
	bool ConfigurePartialLayerFromJointName(const std::string& aRootJointName);

private:

	friend class AnimationState;

	struct PlaybackState
	{
		std::shared_ptr<AnimationAsset> CurrentAnimation;
		std::string AnimationName;
		std::size_t CurrentFrame = 0;
		float Timer = 0.0f;
		bool Looping = true;
		bool Looped = false;
		bool Active = false;
	};

	bool AdvancePlayback(PlaybackState& aPlayback, float aDeltaTime);
	void MarkJointAndChildren(std::size_t aJointIndex);
	void RebuildJointTransforms();
	void UpdateJointPose(std::size_t aJointIndex, const CommonUtilities::Matrix4f& aParentJointTransform);
	const CommonUtilities::Matrix4f& GetLocalTransformForJoint(std::size_t aJointIndex) const;

	SkeletalMeshComponent* myMeshComponent;
	AnimationTree* myAnimationTree;

	PlaybackState myBaseLayer;
	PlaybackState myPartialLayer;

	std::unordered_map<std::string, std::shared_ptr<AnimationAsset>> myAnimations;
	std::array<bool, 128> myPartialLayerMask = {};
};
