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

	return true;
}
