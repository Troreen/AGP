#pragma once
#include "CommonUtilities/CommonUtilities.h"

struct ObjectBuffer
{
    CU::Matrix4f World;
    CU::Matrix4f WorldInvT;
    unsigned HasSkinning = false;
	unsigned IsInstanced = false;
	CU::Vector2f __padding;
};
