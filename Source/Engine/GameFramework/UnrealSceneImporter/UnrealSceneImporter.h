#pragma once
#include <filesystem>
#include "UnrealSceneStructs.h"

class UnrealSceneImporter
{
	public:
		UnrealSceneImporter() = default;
		~UnrealSceneImporter() = default;

		UnrealSceneData ImportScene(std::filesystem::path aJSONPath);

	private:
};
