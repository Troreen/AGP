#include "../AGPTools/StaticMeshArtifact.h"
#include "../AGPTools/StaticMeshArtifactFormat.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace
{
	using namespace AGP::Tools;

	class TemporaryDirectory
	{
	public:
		TemporaryDirectory()
		{
			const auto uniquePart = std::chrono::steady_clock::now().time_since_epoch().count();
			myPath = std::filesystem::temp_directory_path() / ("agp-static-mesh-tests-" + std::to_string(uniquePart));
			std::filesystem::create_directories(myPath);
		}

		~TemporaryDirectory()
		{
			std::error_code ignored;
			std::filesystem::remove_all(myPath, ignored);
		}

		const std::filesystem::path& Path() const { return myPath; }

	private:
		std::filesystem::path myPath;
	};

	struct TestContext
	{
		int Failures = 0;

		void Check(const bool aCondition, const std::string_view aDescription)
		{
			if (!aCondition)
			{
				++Failures;
				std::cerr << "FAILED: " << aDescription << '\n';
			}
		}
	};

	StaticMeshData MakeRepresentativeMesh()
	{
		StaticMeshData mesh;
		mesh.Vertices = {
			{ { -1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.5f, 1.0f }, { 0.5f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { 0.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.5f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f } }
		};
		mesh.Indices = { 0, 1, 2, 0, 2, 3 };
		mesh.Submeshes = { { 0, 0, 4, 3, 0 }, { 0, 3, 4, 3, 2 } };
		return mesh;
	}

	std::vector<std::byte> ReadBytes(const std::filesystem::path& aPath)
	{
		const std::uintmax_t size = std::filesystem::file_size(aPath);
		std::vector<std::byte> bytes(static_cast<std::size_t>(size));
		std::ifstream input(aPath, std::ios::binary);
		input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		return bytes;
	}

	void WriteBytes(const std::filesystem::path& aPath, const std::span<const std::byte> someBytes)
	{
		std::ofstream output(aPath, std::ios::binary | std::ios::trunc);
		output.write(reinterpret_cast<const char*>(someBytes.data()), static_cast<std::streamsize>(someBytes.size()));
	}

	template <typename T>
	void PatchLittleEndian(std::vector<std::byte>& someBytes, const std::size_t anOffset, const T aValue)
	{
		for (std::size_t byteIndex = 0; byteIndex < sizeof(T); ++byteIndex)
		{
			someBytes[anOffset + byteIndex] = static_cast<std::byte>((aValue >> (byteIndex * 8)) & 0xffu);
		}
	}

	bool HasCode(const ArtifactValidationResult& aResult, const std::string_view aCode)
	{
		for (const ArtifactDiagnostic& diagnostic : aResult.Diagnostics)
		{
			if (diagnostic.Code == aCode)
			{
				return true;
			}
		}
		return false;
	}

	void TestRoundTrip(TestContext& aContext, const std::filesystem::path& aPath)
	{
		const StaticMeshData expected = MakeRepresentativeMesh();
		const ArtifactValidationResult write = WriteStaticMeshArtifact(aPath, expected);
		aContext.Check(write.Succeeded(), "writer validates its completed artifact");
		aContext.Check(ValidateStaticMeshArtifact(aPath).Succeeded(), "standalone validator accepts writer output");

		const StaticMeshReadResult read = ReadStaticMeshArtifact(aPath);
		aContext.Check(read.Succeeded(), "reader accepts writer output");
		aContext.Check(read.Mesh.has_value() && *read.Mesh == expected, "artifact round trip preserves mesh data");
	}

	void TestCorruptions(TestContext& aContext, const std::filesystem::path& aValidPath, const std::filesystem::path& aScratchPath)
	{
		const std::vector<std::byte> validBytes = ReadBytes(aValidPath);

		auto corrupted = validBytes;
		corrupted[0] = std::byte{ 0 };
		WriteBytes(aScratchPath, corrupted);
		aContext.Check(HasCode(ValidateStaticMeshArtifact(aScratchPath), "artifact.header.magic"), "validator rejects wrong magic");

		corrupted = validBytes;
		PatchLittleEndian(corrupted, Detail::SchemaVersionOffset, std::uint32_t{ StaticMeshArtifactSchemaVersion + 1 });
		WriteBytes(aScratchPath, corrupted);
		aContext.Check(HasCode(ValidateStaticMeshArtifact(aScratchPath), "artifact.header.version"), "validator rejects unsupported schema");

		corrupted = validBytes;
		PatchLittleEndian(corrupted, Detail::SectionDescriptorsOffset + 8, std::uint64_t{ Detail::HeaderSize + 4 });
		WriteBytes(aScratchPath, corrupted);
		aContext.Check(HasCode(ValidateStaticMeshArtifact(aScratchPath), "artifact.section.range"), "validator rejects malformed section ranges");

		corrupted = validBytes;
		const std::size_t firstIndexOffset = Detail::HeaderSize + MakeRepresentativeMesh().Vertices.size() * Detail::VertexStride;
		PatchLittleEndian(corrupted, firstIndexOffset, std::uint32_t{ 99 });
		WriteBytes(aScratchPath, corrupted);
		aContext.Check(HasCode(ValidateStaticMeshArtifact(aScratchPath), "artifact.index.range"), "validator rejects out-of-range indices");

		corrupted = validBytes;
		corrupted.push_back(std::byte{ 0 });
		PatchLittleEndian(corrupted, Detail::FileSizeOffset, static_cast<std::uint64_t>(corrupted.size()));
		WriteBytes(aScratchPath, corrupted);
		aContext.Check(HasCode(ValidateStaticMeshArtifact(aScratchPath), "artifact.file.trailing_data"), "validator requires complete byte consumption");

		const StaticMeshReadResult rejectedRead = ReadStaticMeshArtifact(aScratchPath);
		aContext.Check(!rejectedRead.Succeeded() && !rejectedRead.Mesh.has_value(), "reader exposes no mesh for an invalid artifact");
	}

	void TestInvalidInput(TestContext& aContext, const std::filesystem::path& aPath)
	{
		StaticMeshData mesh = MakeRepresentativeMesh();
		mesh.Submeshes[0].IndexCount = 4;
		const ArtifactValidationResult result = WriteStaticMeshArtifact(aPath, mesh);
		aContext.Check(!result.Succeeded(), "writer rejects structurally invalid input");
		aContext.Check(HasCode(result, "artifact.submesh.index_range"), "writer returns a structured invalid-range diagnostic");
	}
}

int main()
{
	TestContext context;
	TemporaryDirectory temporaryDirectory;
	const std::filesystem::path validPath = temporaryDirectory.Path() / "roundtrip.agpsmesh";
	const std::filesystem::path scratchPath = temporaryDirectory.Path() / "corrupt.agpsmesh";

	context.Check(!GetStaticMeshToolVersion().empty(), "tool version is stable and non-empty");
	context.Check(StaticMeshArtifactSchemaVersion != 0, "artifact schema version is non-zero");
	TestRoundTrip(context, validPath);
	TestCorruptions(context, validPath, scratchPath);
	TestInvalidInput(context, temporaryDirectory.Path() / "invalid.agpsmesh");

	if (context.Failures != 0)
	{
		std::cerr << context.Failures << " static-mesh artifact test(s) failed.\n";
		return 1;
	}

	std::cout << "All static-mesh artifact tests passed.\n";
	return 0;
}
