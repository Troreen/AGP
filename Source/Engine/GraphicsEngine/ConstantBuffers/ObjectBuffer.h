#pragma once
#include "CommonUtilities/CommonUtilities.h"

struct ObjectBuffer
{
    CU::Matrix4f World;
    CU::Matrix4f WorldInvT;
    unsigned HasSkinning = false;
    CU::Vector3f __padding;
};
