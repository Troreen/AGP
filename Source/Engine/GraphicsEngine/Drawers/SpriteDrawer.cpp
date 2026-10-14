#include "GraphicsEngine.pch.h"
#include "SpriteDrawer.h"

SpriteDrawer& SpriteDrawer::Get()
{
	static SpriteDrawer instance;
	return instance;
}

void SpriteDrawer::AddDrawCall(const SpriteDrawCall& aDrawCall)
{
	if (aDrawCall.SpriteSharedData == nullptr || aDrawCall.SpriteInstanceData == nullptr)
	{
		GELOG(Error, "Sprite was submitted with invalid data!");
		return;
	}

	if (!aDrawCall.SpriteInstanceData->IsVisible)
	{
		return;
	}

	myDrawCalls.emplace_back(aDrawCall);
}

std::vector<SpriteDrawCall> SpriteDrawer::GetDrawCalls()
{
	return myDrawCalls;
}

void SpriteDrawer::Clear()
{
	myDrawCalls.clear();
}
