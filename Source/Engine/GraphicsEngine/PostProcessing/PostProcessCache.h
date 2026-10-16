#pragma once

#include "../RHI/RenderHardwareInterface.h"

class PostProcessCache
{
    friend class PostProcessEffect;

public:
    std::unique_ptr<PipelineStateObject> resamplePSO;

public:
    bool Initialize(const RenderHardwareInterface& aRHI);
};
