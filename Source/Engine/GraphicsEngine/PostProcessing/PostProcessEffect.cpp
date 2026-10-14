#include "GraphicsEngine.pch.h"
#include "PostProcessEffect.h"

PostProcessEffect::PostProcessEffect() = default;
PostProcessEffect::~PostProcessEffect() = default;

bool PostProcessEffect::Initialize([[maybe_unused]] const RenderHardwareInterface& aRHI)
{
    return true;
}

void PostProcessEffect::SetBuffers([[maybe_unused]] GraphicsCommandList& inoutCommandList,
	[[maybe_unused]] GraphicsEngine& aEngine)
{
}

void PostProcessEffect::RunPreLight([[maybe_unused]] GraphicsCommandList& inoutCommandList,
	[[maybe_unused]] PostProcessCache& inoutCache, [[maybe_unused]] const Texture* aBackBuffer)
{
}

void PostProcessEffect::RunPostLight([[maybe_unused]] GraphicsCommandList& inoutCommandList,
	[[maybe_unused]] PostProcessCache& inoutCache, [[maybe_unused]] const Texture* aBackBuffer)
{
}

bool PostProcessEffect::IsActive() const
{
    return myIsActive;
}

void PostProcessEffect::SetActive(bool aActive)
{
    myIsActive = aActive;
}
