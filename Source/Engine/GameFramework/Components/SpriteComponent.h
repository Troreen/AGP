#pragma once
#include "../World/Component.h"
#include <GraphicsEngine/Sprite/SpriteStructs.h>
#include <GameFramework/AssetHandling/TextureAsset.h>

class Texture;

class SpriteComponent : public Component
{
public:

	SpriteComponent();

	void Update(float) override;

	void SendDrawCall();

	void SetSprite(const std::shared_ptr<TextureAsset>& aTextureAsset);

	const SpriteInstanceData& GetInstanceData() const;
	const SpriteSharedData& GetSharedData() const;

	void SetSize(const CommonUtilities::Vector2f& aSize);
	void SetPivot(const CommonUtilities::Vector2f& aPivot);
	void SetAnchor(const CommonUtilities::Vector2f& aAnchor);
	void SetPosition(const CommonUtilities::Vector2f& aPosition);
	void SetColor(const CommonUtilities::Vector4f& aColor);
	void SetDepth(float aDepth);
	void SetVisible(bool aVisible);

private:

	SpriteInstanceData mySpriteInstance;
	std::shared_ptr<TextureAsset> myTextureAsset;
	SpriteSharedData mySharedData;
};

