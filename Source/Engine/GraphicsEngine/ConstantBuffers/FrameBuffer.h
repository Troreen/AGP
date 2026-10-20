#pragma once
#include "CommonUtilities/CommonUtilities.h"
#include <cstddef>

struct FrameBuffer
{
    CU::Matrix4f View;
    CU::Matrix4f Projection;
    CU::Vector3f CameraPosition;
    float TotalTime = 0;
    uint32_t TonemapType = 0;
    uint32_t RenderPassType = 0;
    CU::Vector2f __padding;
    // Materials need the sun before the later lighting pass runs.
    // This points toward the scene's directional light; zero means no active sun.
    CU::Vector3f SunDirectionToLight = { 0, 0, 0 };
    float __SunPadding = 0;
};

// C++ and HLSL must agree about where the sun row starts and how much data is uploaded.
static_assert(offsetof(FrameBuffer, SunDirectionToLight) == 160);
static_assert(sizeof(FrameBuffer) == 176);

