#pragma once
#include "Matrix.hpp"

struct FrameBuffer
{
    CU::Matrix4f View;
    CU::Matrix4f Projection;
};

