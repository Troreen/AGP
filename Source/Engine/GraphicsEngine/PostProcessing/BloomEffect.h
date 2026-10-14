#pragma once

#include "PostProcessEffect.h"

class BloomEffect : public PostProcessEffect
{
public:
    struct Settings
    {
        float LuminanceExponent = 1.f;
        float LuminanceThreshhold = 1.f;
        CommonUtilities::Vector2f __padding;
    };

    ~BloomEffect() override;

    Settings& GetSettings();
    const Settings& GetSettings() const;

protected:
	bool Initialize(const RenderHardwareInterface& aRHI) override;
	void SetBuffers(GraphicsCommandList& inoutCommandList, GraphicsEngine& aEngine) override;
	void RunPostLight(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const Texture* aBackBuffer) override;

private:
    void Blur(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const PipelineStateObject& aBlurPSO, int aCurrentSlot);

    std::unique_ptr<PipelineStateObject> myLuminancePSO;
    std::unique_ptr<PipelineStateObject> myGaussianBlurHPSO;
    std::unique_ptr<PipelineStateObject> myGaussianBlurVPSO;
    std::unique_ptr<PipelineStateObject> myBloomPSO;

    Settings mySettings;
};
