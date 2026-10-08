#include "SpriteComponent.h"
#include <GraphicsEngine/Drawers/SpriteDrawer.h>

SpriteComponent::SpriteComponent()
{
}

void SpriteComponent::Update(float)
{
	SendDrawCall();
}

void SpriteComponent::SendDrawCall()
{
	SpriteDrawCall drawCall =
	{
		.SpriteInstanceData = &mySpriteInstance,
		.SpriteSharedData = &mySharedData,
		.Material = nullptr
	};
	SpriteDrawer::Get().AddDrawCall(drawCall);
}

void SpriteComponent::SetSprite(const std::shared_ptr<TextureAsset>& aTextureAsset)
{
	myTextureAsset = aTextureAsset;
	mySharedData.Texture = myTextureAsset->GetTexture();
}

const SpriteInstanceData& SpriteComponent::GetInstanceData() const
{
	return mySpriteInstance;
}

const SpriteSharedData& SpriteComponent::GetSharedData() const
{
	return mySharedData;
}

void SpriteComponent::SetSize(const CommonUtilities::Vector2f& aSize)
{
	mySpriteInstance.Size = aSize;
}

void SpriteComponent::SetPivot(const CommonUtilities::Vector2f & aPivot)
{
	mySpriteInstance.Pivot = aPivot;
}

void SpriteComponent::SetAnchor(const CommonUtilities::Vector2f & aAnchor)
{
	mySpriteInstance.Anchor = aAnchor;
}

void SpriteComponent::SetPosition(const CommonUtilities::Vector2f & aPosition)
{
	mySpriteInstance.Position = aPosition;
}

void SpriteComponent::SetColor(const CommonUtilities::Vector4f & aColor)
{
	mySpriteInstance.Color = aColor;
}

void SpriteComponent::SetDepth(float aDepth)
{
	mySpriteInstance.Depth = aDepth;
}

void SpriteComponent::SetVisible(bool aVisible)
{
	mySpriteInstance.IsVisible = aVisible;
}
