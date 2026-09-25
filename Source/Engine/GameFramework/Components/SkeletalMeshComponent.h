#pragma once

#include "GameFramework/AssetHandling/AnimationAsset.h"
#include "GameFramework/Components/MeshComponentBase.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

class Animation;

// Per-instance animation playback and joint poses over a shared mesh/skeleton.
// The world advances animation during Update and snapshot extraction copies its pose.
// Attach a controlling component before this component for same-frame play requests.
class SkeletalMeshComponent final : public MeshComponentBase
{
public:
	SkeletalMeshComponent();
	explicit SkeletalMeshComponent(const std::shared_ptr<MeshAsset>& aMesh);

	void Update(float aDeltaTime) override;

	// TODO: Move this to animator component
	void AddAnimation(std::string_view aName, const std::shared_ptr<AnimationAsset>& anAnimation);
	// TODO: Move this to animator component
	std::shared_ptr<AnimationAsset> GetAnimation(std::string_view aName) const;

	bool PlayAnimation(std::string_view anAnimationName, bool aShouldLoop);
	bool PlayPartialAnimation(std::string_view anAnimationName, bool aShouldLoop);
	bool ConfigurePartialLayerFromJointName(std::string_view aRootJointName);

protected:
	void OnMeshChanged() override;

private:
	bool HasSkinning() const override;
	const std::array<CommonUtilities::Matrix4f, 128>* GetJointTransforms() const override;

	struct PlaybackState
	{
		std::shared_ptr<AnimationAsset> CurrentAnimation;
		std::string AnimationName;
		std::size_t CurrentFrame = 0;
		float Timer = 0.0f;
		bool Looping = true;
		bool Active = false;
	};

	void ResetJointTransforms();
	bool AdvancePlayback(PlaybackState& aPlayback, float aDeltaTime);
	void RebuildJointTransforms();
	void UpdateJointPose(std::size_t aJointIndex, const CommonUtilities::Matrix4f& aParentJointTransform);
	const CommonUtilities::Matrix4f& GetLocalTransformForJoint(std::size_t aJointIndex) const;
	void MarkJointAndChildren(std::size_t aJointIndex);

	PlaybackState myBaseLayer;
	PlaybackState myPartialLayer;

	std::unordered_map<std::string, std::shared_ptr<AnimationAsset>> myAnimations;

	std::array<CommonUtilities::Matrix4f, 128> myJointTransforms;
	std::array<bool, 128> myPartialLayerMask = {};
};
