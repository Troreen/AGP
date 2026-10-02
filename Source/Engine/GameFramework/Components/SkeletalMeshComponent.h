#pragma once

#include "GameFramework/AssetHandling/AnimationAsset.h"
#include "GameFramework/Components/MeshComponentBase.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

class Animation;
class AnimatorComponent;

// Per-instance animation playback and joint poses over a shared mesh/skeleton.
// The world advances animation during Update and snapshot extraction copies its pose.
// Attach a controlling component before this component for same-frame play requests.
class SkeletalMeshComponent final : public MeshComponentBase
{
public:
	SkeletalMeshComponent();
	explicit SkeletalMeshComponent(const std::shared_ptr<MeshAsset>& aMesh);

	void Update(float aDeltaTime) override;


protected:
	void OnMeshChanged() override;

private:

	friend class AnimatorComponent;

	bool HasSkinning() const override;

	const std::array<CommonUtilities::Matrix4f, 128>* GetJointTransforms() const override;

	void ResetJointTransforms();

	std::array<CommonUtilities::Matrix4f, 128> myJointTransforms;
};
