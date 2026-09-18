#pragma once
#include "UnrealSceneStructs.h"
#include <filesystem>

class UnrealSceneImporter
{
public:
	UnrealImportResult ImportScene(const std::filesystem::path& jsonPath) const;
};
