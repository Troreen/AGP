#pragma once

#include "StaticMeshArtifact.h"

#include <atomic>
#include <filesystem>
#include <optional>
#include <vector>

namespace AGP::Tools
{
	struct StaticMeshConversionResult
	{
		std::optional<StaticMeshData> Mesh;
		bool Canceled = false;
		std::vector<ArtifactDiagnostic> Diagnostics;

		[[nodiscard]] bool Succeeded() const;
	};

	struct StaticMeshArtifactBuildResult
	{
		bool Completed = false;
		bool Canceled = false;
		std::vector<ArtifactDiagnostic> Diagnostics;

		[[nodiscard]] bool Succeeded() const;
	};

	// TGA's FBX SDK call is not interruptible. Cancellation is checked before the
	// call starts and immediately after it returns. AGPTools serializes FBX imports
	// because the underlying importer owns process-global mutable state.
	[[nodiscard]] StaticMeshConversionResult ConvertFbxToStaticMeshData(
		const std::filesystem::path& aSourcePath,
		const std::atomic_bool& aCancellationRequested);

	// The destination is caller-owned staging. A canceled result must never be
	// committed; if cancellation races with the completed write, the caller removes
	// the staged file.
	[[nodiscard]] StaticMeshArtifactBuildResult BuildStaticMeshArtifactFromFbx(
		const std::filesystem::path& aSourcePath,
		const std::filesystem::path& anArtifactPath,
		const std::atomic_bool& aCancellationRequested);
}
