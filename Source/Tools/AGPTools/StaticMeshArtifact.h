#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace AGP::Tools
{
	inline constexpr std::uint32_t StaticMeshArtifactSchemaVersion = 1;

	struct StaticMeshVertex
	{
		std::array<float, 4> Position = { 0.0f, 0.0f, 0.0f, 1.0f };
		std::array<float, 4> Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		std::array<float, 2> UV0 = { 0.0f, 0.0f };
		std::array<float, 2> UV1 = { 0.0f, 0.0f };
		std::array<float, 3> Normal = { 0.0f, 0.0f, 1.0f };
		std::array<float, 3> Tangent = { 1.0f, 0.0f, 0.0f };

		bool operator==(const StaticMeshVertex&) const = default;
	};

	struct StaticMeshSubmesh
	{
		std::uint32_t VertexOffset = 0;
		std::uint32_t IndexOffset = 0;
		std::uint32_t VertexCount = 0;
		std::uint32_t IndexCount = 0;
		std::uint32_t MaterialIndex = 0;

		bool operator==(const StaticMeshSubmesh&) const = default;
	};

	struct StaticMeshData
	{
		std::vector<StaticMeshVertex> Vertices;
		std::vector<std::uint32_t> Indices;
		std::vector<StaticMeshSubmesh> Submeshes;

		bool operator==(const StaticMeshData&) const = default;
	};

	enum class ArtifactDiagnosticSeverity
	{
		Error,
		Warning,
		Info
	};

	struct ArtifactDiagnostic
	{
		ArtifactDiagnosticSeverity Severity = ArtifactDiagnosticSeverity::Error;
		std::string Code;
		std::string Message;
		std::uint64_t ByteOffset = 0;
	};

	struct ArtifactValidationResult
	{
		std::vector<ArtifactDiagnostic> Diagnostics;

		[[nodiscard]] bool Succeeded() const;
	};

	struct StaticMeshReadResult
	{
		std::optional<StaticMeshData> Mesh;
		std::vector<ArtifactDiagnostic> Diagnostics;

		[[nodiscard]] bool Succeeded() const;
	};

	[[nodiscard]] std::string_view GetStaticMeshToolVersion();
	[[nodiscard]] ArtifactValidationResult ValidateStaticMeshArtifact(const std::filesystem::path& anArtifactPath);
	[[nodiscard]] ArtifactValidationResult WriteStaticMeshArtifact(
		const std::filesystem::path& anArtifactPath,
		const StaticMeshData& aMesh);
	[[nodiscard]] StaticMeshReadResult ReadStaticMeshArtifact(const std::filesystem::path& anArtifactPath);
}
