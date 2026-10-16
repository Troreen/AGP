#pragma once

#include "PostProcessEffect.h"

class SSAOEffect : public PostProcessEffect
{
public:
    struct Settings
    {
		float SamplingRadius = 0.5f;
		float SamplingBias = 0.025f;
		CommonUtilities::Vector2f __padding;
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

	std::vector<Texture> myResizeTextures;

    Texture mySSAOBuffer;

    Settings mySettings;
};
