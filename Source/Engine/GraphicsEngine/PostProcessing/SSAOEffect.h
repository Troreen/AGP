#pragma once

#include "PostProcessEffect.h"

class SSAOEffect : public PostProcessEffect
{
public:
    struct Settings
    {
		float SampleRadiusTexels = 4;
		float MaxDistance = 100;
		float MinDistance = 0.001f;
		float __padding = 0;
    };

    ~SSAOEffect() override;

    Settings& GetSettings();
    const Settings& GetSettings() const;

protected:
	bool Initialize(const RenderHardwareInterface& aRHI) override;
	void SetBuffers(GraphicsCommandList& inoutCommandList, GraphicsEngine& aEngine) override;
	void RunPreLight(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const Texture* aBackBuffer) override;

private:
    std::unique_ptr<PipelineStateObject> myGenerateSSAOPSO;
    std::unique_ptr<PipelineStateObject> mySSAOBlurHPSO;
    std::unique_ptr<PipelineStateObject> mySSAOBlurVPSO;

    Texture mySSAOBuffer;

    Settings mySettings;
};
