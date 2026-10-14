#pragma once

#include <CommonUtilities/CommonUtilities.h>
#include <string>
#include <unordered_map>
#include <vector>

struct Animation
{
    struct Frame
    {
        std::unordered_map<std::string, CU::Matrix4f> Transforms;
    };

    std::vector<Frame> Frames;
    float Duration = 0;
    float FramesPerSecond = 0;
	std::string Name;

    bool Looping = true;

	bool IsValid() const;
};
