#pragma once
#include "../Animation/AnimationTree.h"

#include <GameFramework/World/Component.h>
#include <GameFramework/Components/SkeletalMeshComponent.h>


class AnimatorComponent : public Component
{
	friend class AnimationState;

public:
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

	AnimatorComponent();

	void Update(float aDeltaTime) override;

	void SetTree(const std::string& aTree);
	// Assign during setup. The mesh component must stay alive while this animator uses it;
	// removing it separately at runtime is not supported. Destroy the owning Actor as a unit.
	void SetMeshComponent(SkeletalMeshComponent* aMeshComponent);
	void AddAnimation(const std::string& aName, const std::shared_ptr<AnimationAsset>& anAnimation);
	std::shared_ptr<AnimationAsset> GetAnimation(const std::string& aName) const;
	const PlaybackState& GetCurrentPlayBackState(bool aIsBaseLayer) const;
	AnimationTree& GetTree();

	bool PlayAnimation(const std::string& anAnimationName, bool aShouldLoop);
	bool CanBeginScriptedAnimation(const std::string& anAnimationName) const;
	bool BeginScriptedAnimation(const std::string& anAnimationName, bool aShouldLoop);
	bool IsScriptedAnimationPlaying() const;
	// Restart the tree's current clip, or hold the scripted pose if there is no tree.
	void EndScriptedAnimation();
	bool HasAnimationTree() const;
	bool HasAnimationVariable(const std::string& aName) const;
	bool PlayPartialAnimation(const std::string& anAnimationName, bool aShouldLoop);
	bool ConfigurePartialLayerFromJointName(const std::string& aRootJointName);

private:
	bool AdvancePlayback(PlaybackState& aPlayback, float aDeltaTime);
	void MarkJointAndChildren(std::size_t aJointIndex);
	void RebuildJointTransforms();
	void UpdateJointPose(std::size_t aJointIndex, const CommonUtilities::Matrix4f& aParentJointTransform);
	const CommonUtilities::Matrix4f& GetLocalTransformForJoint(std::size_t aJointIndex) const;

	SkeletalMeshComponent* myMeshComponent;
	AnimationTree* myAnimationTree;

	PlaybackState myBaseLayer;
	PlaybackState myPartialLayer;
	bool myScriptedAnimationOverride = false;

	std::unordered_map<std::string, std::shared_ptr<AnimationAsset>> myAnimations;
	std::array<bool, 128> myPartialLayerMask = {};
};
