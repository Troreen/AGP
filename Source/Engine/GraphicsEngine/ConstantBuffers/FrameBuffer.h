#pragma once

#include "CommonUtilities/CommonUtilities.h"

struct FrameBuffer
{
    CU::Matrix4f View;
    CU::Matrix4f Projection;
    CU::Vector3f CameraPosition;
    float TotalTime = 0;
	float DeltaTime = 0;
    uint32_t TonemapType = 0;
    uint32_t RenderPassType = 0;
	float __padding = 0;
    // Materials need the sun before the later lighting pass runs.
    // This points toward the scene's directional light; zero means no active sun.
    CU::Vector3f SunDirectionToLight = { 0, 0, 0 };
	float __sunPadding = 0;
};

static_assert(sizeof(FrameBuffer) == 176);
