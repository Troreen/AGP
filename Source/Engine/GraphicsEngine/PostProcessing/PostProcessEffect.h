#pragma once

#include "../GraphicsEngine.h"
#include "PostProcessCache.h"
#include "RHI/RenderHardwareInterface.h"

class PostProcessEffect
{
	friend class GraphicsEngine;

public:
	// Maximum allowed size of the HLSL Post Processing constant buffers.
	// This is to make sure we don't underflow data storage on the C++ side.
	// This cannot be more than 65535 (64 kB)! Hard limit for CBuffers.
	static constexpr unsigned POST_PROCESS_BUFFER_SIZE = 128;

	PostProcessEffect();
	virtual ~PostProcessEffect();

	bool IsActive() const;
	void SetActive(bool aActive);

protected:
	virtual bool Initialize(const RenderHardwareInterface& aRHI) = 0;

	virtual void SetBuffers(GraphicsCommandList& inoutCommandList, GraphicsEngine& aEngine);

	virtual void RunPreLight(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const Texture* aBackBuffer);
	virtual void RunPostLight(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const Texture* aBackBuffer);

private:
	bool myIsActive = true;
};
