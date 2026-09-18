#pragma once
#include "GameFramework/Scenes/SceneData.h"
#include "UnrealSceneStructs.h"
#include <optional>

struct UnrealSceneAdapterConfig
{
	float UnitScale = 1.f;
};
struct SceneConversionResult
{
	std::optional<SceneData> Scene;
	std::vector<ImportDiagnostic> Diagnostics;
	explicit operator bool() const { return Scene.has_value() && Diagnostics.empty(); }
};

class UnrealSceneAdapter
{
public:
	SceneConversionResult Convert(const UnrealSceneData& source, const UnrealSceneAdapterConfig& config = {}) const;
};
