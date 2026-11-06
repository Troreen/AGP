#pragma once

#include <CommonUtilities/CommonUtilities.h>

enum class TonemapType : uint8_t
{
    None,
    ACES,
    Unreal,
    Lottes,

    Count
};

enum class RenderPassType : uint8_t
{
    Light,
	Albedo,
    Roughness,
    Metalness,
    AmbientOcclusionTexture,
    Emission,
	AmbientOcclusionScreenSpace,
    NormalLocal,
	VertexNormalWorld,
    NormalWorld,

    Count
};

struct GraphicsRenderSettings
{
    TonemapType TonemapAlgorithm = TonemapType::ACES;
    RenderPassType RenderPass = RenderPassType::Light;
    bool RunPostProcesses = true;

	static const char* GetRenderPassName(RenderPassType aRenderPass);
};
