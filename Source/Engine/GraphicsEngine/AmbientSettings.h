#pragma once

#include "Objects/Texture.h"

#include <CommonUtilities/CommonUtilities.h>

struct AmbientSettings
{
	Texture EnvironmentCubeTexture;

	CU::Vector3f AmbientLightColor = CU::Vector3f(1.0f, 1.0f, 1.0f);
	float AmbientLightIntensity = 1.0f;

	bool ShowSkybox = false;
};
