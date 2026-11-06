#pragma once

#include "PostProcessEffect.h"

class BloomEffect : public PostProcessEffect
{
public:
    struct Settings
    {
		float FilterRadius = 0.0075f;
        float Strength = 0.3f;
		float Threshold = 1.0f;
		float Softness = 0.5f;
    };

    ~BloomEffect() override;

    Settings& GetSettings();
    const Settings& GetSettings() const;

protected:
	bool Initialize(const RenderHardwareInterface& aRHI) override;
	void SetBuffers(GraphicsCommandList& inoutCommandList, GraphicsEngine& aEngine) override;
	void RunPostLight(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const Texture* aBackBuffer) override;

private:
    std::unique_ptr<PipelineStateObject> myDownscalePSO;
    std::unique_ptr<PipelineStateObject> myUpscalePSO;
	std::unique_ptr<PipelineStateObject> myBloom01DownsamplePSO;
	std::unique_ptr<PipelineStateObject> myBloomLuminancePSO;
	std::unique_ptr<PipelineStateObject> myBloomPSO;

	std::vector<Texture> myResizeTextures;

    Settings mySettings;
};
