#pragma once

#include "Asset.h"

#include <GraphicsEngine/Objects/Mesh.h>

class AnimationAsset : public Asset
{
public:
	AnimationAsset();
	explicit AnimationAsset(const std::shared_ptr<Animation>& aAnimation);
	~AnimationAsset() override;

	const std::shared_ptr<Animation>& GetAnimation() const;

protected:
	bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) override;

private:
	std::shared_ptr<Animation> myAnimation;
};
