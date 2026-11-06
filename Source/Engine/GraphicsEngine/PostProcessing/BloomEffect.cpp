#include "GraphicsEngine.pch.h"
#include "BloomEffect.h"

#include "../RHI/GraphicsCommandList.h"
#include "Bloom_PS.h"
#include "Bloom01Downsample_PS.h"
#include "BloomDownsample_PS.h"
#include "BloomLuminance_PS.h"
#include "BloomUpsample_PS.h"
#include "FullTexture_VS.h"

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
		PipelineStateDescription downscaleDesc;
		downscaleDesc.Name = "downscalePSO";
		downscaleDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		downscaleDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		downscaleDesc.PixelShader.ByteCode = INTERNAL_BloomDownsample_PS_ByteCode;
		downscaleDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_BloomDownsample_PS_ByteCode);
		downscaleDesc.BlendMode = BlendMode::Opaque;
		downscaleDesc.Topology = Topology::TriangleStrip;

		myDownscalePSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(downscaleDesc, *myDownscalePSO))
		{
			GELOG(Error, "Failed to create bloom downscale PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription upscaleDesc;
		upscaleDesc.Name = "upscalePSO";
		upscaleDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		upscaleDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		upscaleDesc.PixelShader.ByteCode = INTERNAL_BloomUpsample_PS_ByteCode;
		upscaleDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_BloomUpsample_PS_ByteCode);
		upscaleDesc.BlendMode = BlendMode::Opaque;
		upscaleDesc.Topology = Topology::TriangleStrip;

		myUpscalePSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(upscaleDesc, *myUpscalePSO))
		{
			GELOG(Error, "Failed to create bloom upscale PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription bloomLuminanceDesc;
		bloomLuminanceDesc.Name = "BloomLuminancePSO";
		bloomLuminanceDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		bloomLuminanceDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		bloomLuminanceDesc.PixelShader.ByteCode = INTERNAL_BloomLuminance_PS_ByteCode;
		bloomLuminanceDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_BloomLuminance_PS_ByteCode);
		bloomLuminanceDesc.BlendMode = BlendMode::Opaque;
		bloomLuminanceDesc.Topology = Topology::TriangleStrip;

		myBloomLuminancePSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(bloomLuminanceDesc, *myBloomLuminancePSO))
		{
			GELOG(Error, "Failed to create bloom luminance PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription bloom01DownsamplePSO;
		bloom01DownsamplePSO.Name = "Bloom01DownsamplePSO";
		bloom01DownsamplePSO.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		bloom01DownsamplePSO.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		bloom01DownsamplePSO.PixelShader.ByteCode = INTERNAL_Bloom01Downsample_PS_ByteCode;
		bloom01DownsamplePSO.PixelShader.ByteCodeSize = sizeof(INTERNAL_Bloom01Downsample_PS_ByteCode);
		bloom01DownsamplePSO.BlendMode = BlendMode::Opaque;
		bloom01DownsamplePSO.Topology = Topology::TriangleStrip;

		myBloom01DownsamplePSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(bloom01DownsamplePSO, *myBloom01DownsamplePSO))
		{
			GELOG(Error, "Failed to create bloom mip 0 to 1 downsample PSO!");
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
		bloomDesc.BlendMode = BlendMode::Additive;
		bloomDesc.Topology = Topology::TriangleStrip;

		myBloomPSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(bloomDesc, *myBloomPSO))
		{
			GELOG(Error, "Failed to create bloom PSO!");
			return false;
		}
	}

	const CommonUtilities::Vector2u clientSize = aRHI.GetClientSize();
	myResizeTextures.resize(4);
	unsigned int divisor = 1;

	for (size_t i = 0; i < 4; ++i)
	{
		const std::string name = "Resize Texture " + std::to_string(i);
		const unsigned int width = clientSize.x / divisor;
		const unsigned int height = clientSize.y / divisor;

		if (!aRHI.CreateTexture(name, width, height, 1, ResourceFormat::R11G11B10_FLOAT, ResourceBindFlag_RenderTarget | ResourceBindFlag_ShaderResource, myResizeTextures[i]))
		{
			GELOG(Error, "Failed to create Bloom Resize texture!");
			return false;
		}

		divisor *= 2;
	}

	return true;
}

void BloomEffect::SetBuffers(GraphicsCommandList& inoutCommandList, GraphicsEngine& aEngine)
{
	aEngine.UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::PostProcessBuffer, mySettings, 5, PipelineStage_PixelShader);
}

void BloomEffect::RunPostLight(GraphicsCommandList& inoutCommandList, [[maybe_unused]] PostProcessCache& inoutCache, const Texture* aBackBuffer)
{
	inoutCommandList.BeginEvent("Bloom");

	const Texture* resource = aBackBuffer;
	const Texture* target = &myResizeTextures[0];

	inoutCommandList.SetRenderTarget(&target, 1, nullptr);
	inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);

	inoutCommandList.SetPipelineState(*myBloomLuminancePSO);
	inoutCommandList.Draw(4);

	resource = &myResizeTextures[0];
	target = &myResizeTextures[1];

	inoutCommandList.SetRenderTarget(&target, 1, nullptr);
	inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);

	inoutCommandList.SetPipelineState(*myBloom01DownsamplePSO);
	inoutCommandList.Draw(4);

	inoutCommandList.SetPipelineState(*myDownscalePSO);

	for (size_t i = 2; i < myResizeTextures.size(); ++i)
	{
		resource = &myResizeTextures[i - 1];
		target = &myResizeTextures[i];

		inoutCommandList.SetRenderTarget(&target, 1, nullptr);
		inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);

		inoutCommandList.Draw(4);
	}

	resource = nullptr;
	inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);

	inoutCommandList.SetPipelineState(*myUpscalePSO);
	for (int i = static_cast<int>(myResizeTextures.size()) - 2; i >= 0; --i)
	{
		resource = &myResizeTextures[i + 1];
		target = &myResizeTextures[i];

		inoutCommandList.SetRenderTarget(&target, 1, nullptr);
		inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);

		inoutCommandList.Draw(4);
	}

	inoutCommandList.SetRenderTarget(&aBackBuffer, 1, nullptr);
	resource = &myResizeTextures[0];
	inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);
	inoutCommandList.SetPipelineState(*myBloomPSO);
	inoutCommandList.Draw(4);

	inoutCommandList.EndEvent(); // Bloom
}
