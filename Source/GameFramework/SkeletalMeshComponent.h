#pragma once

#include "MeshComponentBase.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>

struct Animation;

class SkeletalMeshComponent final : public MeshComponentBase
{
public:
	SkeletalMeshComponent();
	explicit SkeletalMeshComponent(std::shared_ptr<Mesh> aMesh);

	void Update(float aDeltaTime) override;

	bool HasSkinning() const override;
	const std::array<CU::Matrix4f, 128>* GetJointTransforms() const override;

	bool PlayAnimation(std::string_view anAnimationName, bool aShouldLoop);
	bool PlayPartialAnimation(std::string_view anAnimationName, bool aShouldLoop);
	bool ConfigurePartialLayerFromJointName(std::string_view aRootJointName);

protected:
	void OnMeshChanged() override;

private:
	struct PlaybackState
	{
		std::shared_ptr<Animation> CurrentAnimation;
		std::string AnimationName;
		size_t CurrentFrame = 0;
		float Timer = 0.0f;
		bool Looping = true;
		bool Active = false;
	};

	void ResetJointTransforms();
	bool AdvancePlayback(PlaybackState& aPlayback, float aDeltaTime);
	void RebuildJointTransforms();
	void UpdateJointPose(size_t aJointIndex, const CU::Matrix4f& aParentJointTransform);
	const CU::Matrix4f& GetLocalTransformForJoint(size_t aJointIndex) const;
	void MarkJointAndChildren(size_t aJointIndex);

	PlaybackState myBaseLayer;
	PlaybackState myPartialLayer;
	std::array<CU::Matrix4f, 128> myJointTransforms;
	std::array<bool, 128> myPartialLayerMask = {};
};
