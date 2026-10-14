#include "GraphicsEngine.pch.h"
#include "SSAOEffect.h"

#include "../RHI/GraphicsCommandList.h"
#include "FullTexture_VS.h"
#include "SSAO_PS.h"
#include "SSAO_BlurH_PS.h"
#include "SSAO_BlurV_PS.h"

SSAOEffect::~SSAOEffect() = default;

SSAOEffect::Settings& SSAOEffect::GetSettings()
{
	return mySettings;
}

const SSAOEffect::Settings& SSAOEffect::GetSettings() const
{
	return mySettings;
}

bool SSAOEffect::Initialize(const RenderHardwareInterface& aRHI)
{
	{
		PipelineStateDescription generateSSAODesc;
		generateSSAODesc.Name = "generateSSAOPSO";
		generateSSAODesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		generateSSAODesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		generateSSAODesc.PixelShader.ByteCode = INTERNAL_SSAO_PS_ByteCode;
		generateSSAODesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_SSAO_PS_ByteCode);
		generateSSAODesc.BlendMode = BlendMode::Opaque;
		generateSSAODesc.Topology = Topology::TriangleStrip;

		myGenerateSSAOPSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(generateSSAODesc, *myGenerateSSAOPSO))
		{
			GELOG(Error, "Failed to create SSAO generation PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription ssaoBlurHDesc;
		ssaoBlurHDesc.Name = "SSAOBlurHPSO";
		ssaoBlurHDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		ssaoBlurHDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		ssaoBlurHDesc.PixelShader.ByteCode = INTERNAL_SSAO_BlurH_PS_ByteCode;
		ssaoBlurHDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_SSAO_BlurH_PS_ByteCode);
		ssaoBlurHDesc.BlendMode = BlendMode::Opaque;
		ssaoBlurHDesc.Topology = Topology::TriangleStrip;

		mySSAOBlurHPSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(ssaoBlurHDesc, *mySSAOBlurHPSO))
		{
			GELOG(Error, "Failed to create SSAO Blur H PSO!");
			return false;
		}
	}

	{
		PipelineStateDescription ssaoBlurVDesc;
		ssaoBlurVDesc.Name = "SSAOBlurVPSO";
		ssaoBlurVDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		ssaoBlurVDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		ssaoBlurVDesc.PixelShader.ByteCode = INTERNAL_SSAO_BlurV_PS_ByteCode;
		ssaoBlurVDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_SSAO_BlurV_PS_ByteCode);
		ssaoBlurVDesc.BlendMode = BlendMode::Opaque;
		ssaoBlurVDesc.Topology = Topology::TriangleStrip;

		mySSAOBlurVPSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(ssaoBlurVDesc, *mySSAOBlurVPSO))
		{
			GELOG(Error, "Failed to create SSAO Blur V PSO!");
			return false;
		}
	}

	const CommonUtilities::Vector2u clientSize = aRHI.GetClientSize();
	const unsigned int width = clientSize.x / 2;
	const unsigned int height = clientSize.y / 2;
	if (!aRHI.CreateTexture("SSAO Buffer", width, height, 1, ResourceFormat::R8_UNORM, ResourceBindFlag_RenderTarget | ResourceBindFlag_ShaderResource, mySSAOBuffer))
	{
		GELOG(Error, "Failed to create SSAO texture!");
		return false;
	}

	return true;
}

void SSAOEffect::SetBuffers(GraphicsCommandList& inoutCommandList, GraphicsEngine& aEngine)
{
	aEngine.UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::PostProcessBuffer, mySettings, 5, PipelineStage_PixelShader);
}

void SSAOEffect::RunPreLight(GraphicsCommandList& inoutCommandList, PostProcessCache& inoutCache, const Texture* aBackBuffer)
{
	inoutCommandList.BeginEvent("SSAO");

	const Texture* ssaoBuffer = &inoutCache.resizeTextures[0];
	inoutCommandList.SetRenderTarget(&ssaoBuffer, 1, nullptr);

	inoutCommandList.SetPipelineState(*myGenerateSSAOPSO);
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

	inoutCommandList.SetPipelineState(*mySSAOBlurHPSO);
	inoutCommandList.Draw(4);

	resource = &inoutCache.resizeTextures[inoutCache.resizeTextures.size() - 1];
	target = &inoutCache.resizeTextures[inoutCache.resizeTextures.size() - 2];
	inoutCommandList.SetRenderTarget(nullptr, 0, nullptr);
	inoutCommandList.SetShaderResources(&resource, 1, 0, PipelineStage_PixelShader);
	inoutCommandList.SetRenderTarget(&target, 1, nullptr);
	inoutCommandList.SetPipelineState(*mySSAOBlurVPSO);
	inoutCommandList.Draw(4);

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
	inoutCommandList.SetShaderResources(&ssaoBuffer, 1, 80, PipelineStage_PixelShader);

	inoutCommandList.EndEvent(); // SSAO
}
