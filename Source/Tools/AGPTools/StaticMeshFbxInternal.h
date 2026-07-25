#pragma once

#include "StaticMeshFbx.h"

#include "Importer.h"

#include <functional>
#include <string>

namespace AGP::Tools::Detail
{
	using FbxMeshLoadFunction = std::function<bool(
		const std::filesystem::path&,
		TGA::FBX::Mesh&,
		std::string&)>;

	[[nodiscard]] StaticMeshConversionResult ConvertImportedStaticMesh(
		const TGA::FBX::Mesh& anImportedMesh,
		const std::filesystem::path& aSourcePath);

	[[nodiscard]] StaticMeshConversionResult ConvertFbxToStaticMeshDataWithLoader(
		const std::filesystem::path& aSourcePath,
		const std::atomic_bool& aCancellationRequested,
		const FbxMeshLoadFunction& aLoadMesh);
}
