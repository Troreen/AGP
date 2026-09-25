#pragma once
#include "GameFramework/Scenes/SceneData.h"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct ImportDiagnostic
{
	std::string Context;
	std::string Message;
};

struct UnrealImportResult
{
	std::optional<SceneData> Data;
	std::vector<ImportDiagnostic> Diagnostics;

	explicit operator bool() const
	{
		return Data.has_value() && Diagnostics.empty();
	}
};

class UnrealSceneImporter
{
public:
	UnrealImportResult ImportScene(const std::filesystem::path& jsonPath) const;
};
