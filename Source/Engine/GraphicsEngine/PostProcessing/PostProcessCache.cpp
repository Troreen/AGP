#include "GraphicsEngine.pch.h"
#include "PostProcessCache.h"

#include "FullTexture_VS.h"
#include "Resample_PS.h"

bool PostProcessCache::Initialize(const RenderHardwareInterface& aRHI)
{
	{
		PipelineStateDescription resampleDesc;
		resampleDesc.Name = "resamplePSO";
		resampleDesc.VertexShader.ByteCode = INTERNAL_FullTexture_VS_ByteCode;
		resampleDesc.VertexShader.ByteCodeSize = sizeof(INTERNAL_FullTexture_VS_ByteCode);
		resampleDesc.PixelShader.ByteCode = INTERNAL_Resample_PS_ByteCode;
		resampleDesc.PixelShader.ByteCodeSize = sizeof(INTERNAL_Resample_PS_ByteCode);
		resampleDesc.BlendMode = BlendMode::Opaque;
		resampleDesc.Topology = Topology::TriangleStrip;

		resamplePSO = std::make_unique<PipelineStateObject>();
		if (!aRHI.CreatePipelineStateObject(resampleDesc, *resamplePSO))
		{
			GELOG(Error, "Failed to create resample PSO!");
			return false;
		}
	}

	const CommonUtilities::Vector2u clientSize = aRHI.GetClientSize();

	resizeTextures.resize(4);
	unsigned int divisor = 1;

	for (size_t i = 0; i < 4; ++i)
	{
		const std::string name = "Resize Texture " + std::to_string(i);
		const unsigned int width = clientSize.x / divisor;
		const unsigned int height = clientSize.y / divisor;

		if (!aRHI.CreateTexture(name, width, height, 1, ResourceFormat::R16G16B16A16_FLOAT, ResourceBindFlag_RenderTarget | ResourceBindFlag_ShaderResource, resizeTextures[i]))
		{
			GELOG(Error, "Failed to create Scene Resize texture!");
			return false;
		}

		divisor *= divisor < 4 ? 2 : 1;
	}

	return true;
}
