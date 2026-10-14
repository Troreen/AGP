#pragma once
#include "CommonUtilities/CommonUtilities.h"

struct FrameBuffer
{
    CU::Matrix4f View;
    CU::Matrix4f Projection;
    CU::Vector3f CameraPosition;
    float TotalTime = 0;
    uint32_t TonemapType = 0;
    uint32_t RenderPassType = 0;
    CU::Vector2f __padding;
};

