#include "AnimationAsset.h"

#include "AssetLoadingHelper.h"

#include <Importer.h>

AnimationAsset::AnimationAsset() = default;

AnimationAsset::AnimationAsset(const std::shared_ptr<Animation>& aAnimation) : myAnimation(aAnimation)
{
}

AnimationAsset::~AnimationAsset() = default;

const std::shared_ptr<Animation>& AnimationAsset::GetAnimation() const
{
	return myAnimation;
}

bool AnimationAsset::Load(const std::filesystem::path& aPath, [[maybe_unused]] AssetRegistry& aRegistry)
{
	TGA::FBX::Animation importedAnimation;
	if (!TGA::FBX::Importer::LoadAnimation(aPath.wstring(), importedAnimation))
	{
		return false;
	}

	std::shared_ptr<Animation> animation = AssetHelper::ConvertAnimation(importedAnimation);
	if (animation == nullptr || !animation->IsValid())
	{
		return false;
	}

	myAnimation = std::move(animation);
	return true;
}
