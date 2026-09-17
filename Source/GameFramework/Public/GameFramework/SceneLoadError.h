#pragma once
#include "GameFramework/SceneId.h"
#include "GameFramework/SceneDiagnostic.h"

struct SceneLoadError
{
	SceneId Scene;
	SceneDiagnostics Diagnostics;
};
