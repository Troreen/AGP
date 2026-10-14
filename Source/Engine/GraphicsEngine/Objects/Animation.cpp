#include "GraphicsEngine.pch.h"

#include "Animation.h"

bool Animation::IsValid() const
{
	return !Frames.empty() && FramesPerSecond > 0.0f;
}
