#pragma once
#include "Matrix.hpp"
#include "Vector.hpp"

struct FrameBuffer
{
    CU::Matrix4f View;
    CU::Matrix4f Projection;
    CU::Vector4f CameraPosition = { 0.0f, 0.0f, 0.0f, 0.0f };
};

