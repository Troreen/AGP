#include "GraphicsEngine.pch.h"
#include "BloomEffect.h"

#include "../RHI/GraphicsCommandList.h"
#include "Bloom_PS.h"
#include "FullTexture_VS.h"
#include "GaussianBlurH_PS.h"
#include "GaussianBlurV_PS.h"
#include "Luminance_PS.h"

BloomEffect::~BloomEffect() = default;

BloomEffect::Settings& BloomEffect::GetSettings()
{
	return mySettings;
}

const BloomEffect::Settings& BloomEffect::GetSettings() const
{
	return mySettings;
}

bool BloomEffect::Initialize(const RenderHardwareInterface& aRHI)
{
	{
		PipelineStateDescription luminanceDesc;
		luminanceDesc.Name = "luminancePSO";
		luminanceDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		luminanceDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		luminanceDesc.PixelShader.ByteCode = INTERNAL_Luminance_PS_ByteCode;
		luminanceDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Luminance_PS_ByteCode);
		luminanceDesc.BlendMode = BlendMode::Opaque;
		luminanceDesc.Topology = Topology::TriangleStrip;

		myLuminancePSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(luminanceDesc, *myLuminancePSO))
		{
			GELOG(Error, "Failed to create luminance PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription gaussianHDesc;
		gaussianHDesc.Name = "GaussianBlurHPSO";
		gaussianHDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		gaussianHDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		gaussianHDesc.PixelShader.ByteCode = INTERNAL_GaussianBlurH_PS_ByteCode;
		gaussianHDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_GaussianBlurH_PS_ByteCode);
		gaussianHDesc.BlendMode = BlendMode::Opaque;
		gaussianHDesc.Topology = Topology::TriangleStrip;

		myGaussianBlurHPSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(gaussianHDesc, *myGaussianBlurHPSO))
		{
			GELOG(Error, "Failed to create gaussian blur H PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription gaussianVDesc;
		gaussianVDesc.Name = "GaussianBlurVPSO";
		gaussianVDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		gaussianVDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		gaussianVDesc.PixelShader.ByteCode = INTERNAL_GaussianBlurV_PS_ByteCode;
		gaussianVDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_GaussianBlurV_PS_ByteCode);
		gaussianVDesc.BlendMode = BlendMode::Opaque;
		gaussianVDesc.Topology = Topology::TriangleStrip;

		myGaussianBlurVPSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(gaussianVDesc, *myGaussianBlurVPSO))
		{
			GELOG(Error, "Failed to create gaussian blur V PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription bloomDesc;
		bloomDesc.Name = "BloomPSO";
		bloomDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		bloomDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		bloomDesc.PixelShader.ByteCode = INTERNAL_Bloom_PS_ByteCode;
		bloomDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Bloom_PS_ByteCode);
		bloomDesc.BlendMode = BlendMode::Alpha;
		bloomDesc.Topology = Topology::TriangleStrip;

		myBloomPSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(bloomDesc, *myBloomPSO))
		{
			GELOG(Error, "Failed to create bloom PSO!");
			return false;
		}
	}

	return true;
}

void BloomEffect::SetBuffers(GraphicsCommandList& inoutCommandList, GraphicsEngine& aEngine)
{
	aEngine.UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::PostProcessBuffer, mySettings, 5, PipelineStage_PixelShader);
}

void BloomEffect::RunPostLight(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const Texture* aBackBuffer)
{
	inoutCommandList.BeginEvent("Bloom");

	const Texture* sceneLuminance = &inoutCache.resizeTextures[0];
	inoutCommandList.SetRenderTarget(&sceneLuminance, 1, nullptr);
	inoutCommandList.SetShaderResources(&aBackBuffer, 1, 0, PipelineStage_PixelShader);

	inoutCommandList.SetPipelineState(*myLuminancePSO);
	inoutCommandList.Draw(4);

	inoutCommandList.SetPipelineState(*inoutCache.resamplePSO);

	const Texture* resource = nullptr;
	const Texture* target = nullptr;
	for (size_t i = 1; i < inoutCache.resizeTextures.size(); ++i)
	{
		resource = &inoutCache.resizeTextures[i - 1];
		target = &inoutCache.resizeTextures[i];

		inoutCommandList.SetRenderTarget(&target, 1, nullptr);
		inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);

		inoutCommandList.Draw(4);
	}

	inoutCommandList.SetPipelineState(*myGaussianBlurHPSO);
	inoutCommandList.Draw(4);

	Blur(inoutCommandList, inoutCache, *myGaussianBlurVPSO, 0);
	Blur(inoutCommandList, inoutCache, *myGaussianBlurHPSO, 1);
	Blur(inoutCommandList, inoutCache, *myGaussianBlurVPSO, 0);

	inoutCommandList.SetPipelineState(*inoutCache.resamplePSO);
	for (int i = static_cast<int>(inoutCache.resizeTextures.size()) - 3; i >= 0; --i)
	{
		resource = &inoutCache.resizeTextures[i + 1];
		target = &inoutCache.resizeTextures[i];

		inoutCommandList.SetRenderTarget(&target, 1, nullptr);
		inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);

		inoutCommandList.Draw(4);
	}

	inoutCommandList.SetRenderTarget(&aBackBuffer, 1, nullptr);
	inoutCommandList.SetShaderResources(&sceneLuminance, 1, 0, PipelineStage_PixelShader);
	inoutCommandList.SetPipelineState(*myBloomPSO);
	inoutCommandList.Draw(4);

	inoutCommandList.EndEvent(); // Bloom
}

void BloomEffect::Blur(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const PipelineStateObject& aBlurPSO, int aCurrentSlot)
{
	const Texture* resource = &inoutCache.resizeTextures[inoutCache.resizeTextures.size() - 1 - aCurrentSlot];
	const Texture* target = &inoutCache.resizeTextures[inoutCache.resizeTextures.size() - 2 + aCurrentSlot];
	inoutCommandList.SetRenderTarget(nullptr, 0, nullptr);
	inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);
	inoutCommandList.SetRenderTarget(&target, 1, nullptr);

	inoutCommandList.SetPipelineState(aBlurPSO);
	inoutCommandList.Draw(4);
}
