#include "GraphicsEngine.pch.h"

const char* GraphicsRenderSettings::GetRenderPassName(RenderPassType aRenderPass)
{
	switch (aRenderPass)
	{
		case RenderPassType::Light:
			return "Lit";

		case RenderPassType::Albedo:
			return "Albedo (sRGB)";

		case RenderPassType::Roughness:
			return "Roughness (Linear Greyscale)";

		case RenderPassType::Metalness:
			return "Metalness (Linear Greyscale)";

		case RenderPassType::AmbientOcclusionTexture:
			return "Ambient Occlusion (Texture, Linear Greyscale)";

		case RenderPassType::Emission:
			return "Emission (Linear Greyscale)";

		case RenderPassType::AmbientOcclusionScreenSpace:
			return "Ambient Occlusion (Screen Space, Linear Greyscale)";

		case RenderPassType::NormalLocal:
			return "Normals (Local Space, Linear)";

		case RenderPassType::NormalWorld:
			return "Normals (World Space, Linear)";

		/*case RenderPassType::Shadows:
			return "Shadows (Directional)";*/

		default:
			return "Unknown";
	}
}
