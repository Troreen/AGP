#pragma once

#include <CommonUtilities/CommonUtilities.h>

struct TextOverlayBuffer
{
	CU::Vector2f ClientSize;
	CU::Vector2f PixelOrigin;

	float Opacity = 1.0f;
	float DistanceRange = 0.0f;

	CU::Vector2f AtlasSize;
};
