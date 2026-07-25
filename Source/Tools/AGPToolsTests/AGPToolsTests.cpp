#include "../AGPTools/StaticMeshArtifact.h"
#include "../AGPTools/StaticMeshArtifactFormat.h"
#include "../AGPTools/StaticMeshFbx.h"
#include "../AGPTools/StaticMeshFbxInternal.h"

#include <atomic>
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

	bool HasCode(const std::vector<ArtifactDiagnostic>& someDiagnostics, const std::string_view aCode)
	{
		for (const ArtifactDiagnostic& diagnostic : someDiagnostics)
		{
			if (diagnostic.Code == aCode)
			{
				return true;
			}
		}
		return false;
	}

	bool HasCode(const ArtifactValidationResult& aResult, const std::string_view aCode)
	{
		return HasCode(aResult.Diagnostics, aCode);
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

	void TestChestFbx(TestContext& aContext, const std::filesystem::path& aTemporaryDirectory)
	{
		const std::filesystem::path repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
		const std::filesystem::path chestPath = repoRoot / "Assets/Meshes/Props/SM_Chest.fbx";
		const std::filesystem::path artifactPath = aTemporaryDirectory / "SM_Chest.agpmesh";
		std::atomic_bool cancellationRequested = false;

		const StaticMeshConversionResult firstConversion = ConvertFbxToStaticMeshData(chestPath, cancellationRequested);
		aContext.Check(firstConversion.Succeeded(), "real chest FBX converts through the public AGPTools API");
		if (!firstConversion.Succeeded())
		{
			for (const ArtifactDiagnostic& diagnostic : firstConversion.Diagnostics)
			{
				std::cerr << diagnostic.Code << ": " << diagnostic.Message << '\n';
			}
			return;
		}

		aContext.Check(!firstConversion.Mesh->Vertices.empty(), "chest conversion produces vertices");
		aContext.Check(!firstConversion.Mesh->Indices.empty() && firstConversion.Mesh->Indices.size() % 3 == 0, "chest conversion produces triangle indices");
		aContext.Check(!firstConversion.Mesh->Submeshes.empty(), "chest conversion produces submeshes");
		aContext.Check(firstConversion.Mesh->Vertices.size() == 3679, "chest fixture has the expected deterministic vertex count");
		aContext.Check(firstConversion.Mesh->Indices.size() == 12201, "chest fixture has the expected deterministic index count");
		aContext.Check(firstConversion.Mesh->Submeshes == std::vector<StaticMeshSubmesh>{
			{ 0, 0, 2237, 7926, 0 },
			{ 2237, 7926, 1442, 4275, 0 }
		}, "chest fixture has the expected deterministic submesh ranges");
		aContext.Check(firstConversion.Mesh->Submeshes.front().VertexOffset == 0, "chest first submesh starts at vertex zero");
		aContext.Check(firstConversion.Mesh->Submeshes.front().IndexOffset == 0, "chest first submesh starts at index zero");

		const StaticMeshConversionResult secondConversion = ConvertFbxToStaticMeshData(chestPath, cancellationRequested);
		aContext.Check(secondConversion.Succeeded() && secondConversion.Mesh == firstConversion.Mesh, "chest FBX conversion is structurally deterministic");

		const StaticMeshArtifactBuildResult build = BuildStaticMeshArtifactFromFbx(chestPath, artifactPath, cancellationRequested);
		aContext.Check(build.Succeeded(), "public FBX-to-artifact operation succeeds for the chest");
		aContext.Check(ValidateStaticMeshArtifact(artifactPath).Succeeded(), "chest artifact passes standalone structural validation");
		const StaticMeshReadResult read = ReadStaticMeshArtifact(artifactPath);
		aContext.Check(read.Succeeded() && read.Mesh == firstConversion.Mesh, "chest artifact reads back to the converted static mesh");
	}

	void TestFbxFailuresAndCancellation(TestContext& aContext, const std::filesystem::path& aTemporaryDirectory)
	{
		std::atomic_bool cancellationRequested = false;
		const std::filesystem::path missingPath = aTemporaryDirectory / "missing.fbx";
		const StaticMeshConversionResult missing = ConvertFbxToStaticMeshData(missingPath, cancellationRequested);
		aContext.Check(!missing.Succeeded() && HasCode(missing.Diagnostics, "fbx.source.missing"), "missing FBX returns a stable source diagnostic");
		aContext.Check(!missing.Diagnostics.empty() && missing.Diagnostics.front().SourcePath == missingPath, "FBX diagnostics retain source context");

		const std::filesystem::path malformedPath = aTemporaryDirectory / "malformed.fbx";
		const std::array malformedBytes = { std::byte{ 'n' }, std::byte{ 'o' }, std::byte{ 't' }, std::byte{ 'f' }, std::byte{ 'b' }, std::byte{ 'x' } };
		WriteBytes(malformedPath, malformedBytes);
		const StaticMeshConversionResult malformed = ConvertFbxToStaticMeshData(malformedPath, cancellationRequested);
		aContext.Check(!malformed.Succeeded() && (HasCode(malformed.Diagnostics, "fbx.import.failed") || HasCode(malformed.Diagnostics, "fbx.import.exception")), "malformed FBX is rejected without exposing mesh data");

		const std::filesystem::path fakePath = aTemporaryDirectory / "fake.fbx";
		WriteBytes(fakePath, malformedBytes);
		const auto emptyLoader = [](const std::filesystem::path&, TGA::FBX::Mesh&, std::string&) { return true; };
		const StaticMeshConversionResult empty = Detail::ConvertFbxToStaticMeshDataWithLoader(fakePath, cancellationRequested, emptyLoader);
		aContext.Check(!empty.Succeeded() && HasCode(empty.Diagnostics, "fbx.mesh.empty"), "empty imported mesh is rejected");

		const auto skeletalLoader = [](const std::filesystem::path&, TGA::FBX::Mesh& outMesh, std::string&)
		{
			outMesh.Elements.emplace_back();
			outMesh.Skeleton.Bones.emplace_back();
			return true;
		};
		const StaticMeshConversionResult skeletal = Detail::ConvertFbxToStaticMeshDataWithLoader(fakePath, cancellationRequested, skeletalLoader);
		aContext.Check(!skeletal.Succeeded() && HasCode(skeletal.Diagnostics, "fbx.mesh.skeletal_unsupported"), "skeletal mesh is explicitly unsupported");

		cancellationRequested.store(true);
		const std::filesystem::path canceledArtifact = aTemporaryDirectory / "canceled.agpmesh";
		const StaticMeshArtifactBuildResult canceledBuild = BuildStaticMeshArtifactFromFbx(fakePath, canceledArtifact, cancellationRequested);
		aContext.Check(canceledBuild.Canceled && HasCode(canceledBuild.Diagnostics, "tool.canceled") && !std::filesystem::exists(canceledArtifact), "public artifact operation honors cancellation before import without writing output");

		bool loaderInvoked = false;
		const auto shouldNotRun = [&loaderInvoked](const std::filesystem::path&, TGA::FBX::Mesh&, std::string&)
		{
			loaderInvoked = true;
			return true;
		};
		const StaticMeshConversionResult canceledBefore = Detail::ConvertFbxToStaticMeshDataWithLoader(fakePath, cancellationRequested, shouldNotRun);
		aContext.Check(canceledBefore.Canceled && HasCode(canceledBefore.Diagnostics, "tool.canceled") && !loaderInvoked, "cancellation before import skips the FBX call");

		cancellationRequested.store(false);
		const auto cancelAfterLoad = [&cancellationRequested](const std::filesystem::path&, TGA::FBX::Mesh& outMesh, std::string&)
		{
			outMesh.Elements.emplace_back();
			cancellationRequested.store(true);
			return true;
		};
		const StaticMeshConversionResult canceledAfter = Detail::ConvertFbxToStaticMeshDataWithLoader(fakePath, cancellationRequested, cancelAfterLoad);
		aContext.Check(canceledAfter.Canceled && HasCode(canceledAfter.Diagnostics, "tool.canceled") && !canceledAfter.Mesh.has_value(), "cancellation after a non-interruptible import discards imported data");
	}
}

int main()
{
	TestContext context;
	TemporaryDirectory temporaryDirectory;
	const std::filesystem::path validPath = temporaryDirectory.Path() / "roundtrip.agpmesh";
	const std::filesystem::path scratchPath = temporaryDirectory.Path() / "corrupt.agpmesh";

	context.Check(!GetStaticMeshToolVersion().empty(), "tool version is stable and non-empty");
	context.Check(StaticMeshArtifactSchemaVersion != 0, "artifact schema version is non-zero");
	TestRoundTrip(context, validPath);
	TestCorruptions(context, validPath, scratchPath);
	TestInvalidInput(context, temporaryDirectory.Path() / "invalid.agpmesh");
	TestChestFbx(context, temporaryDirectory.Path());
	TestFbxFailuresAndCancellation(context, temporaryDirectory.Path());

	if (context.Failures != 0)
	{
		std::cerr << context.Failures << " static-mesh artifact test(s) failed.\n";
		return 1;
	}

	std::cout << "All static-mesh artifact tests passed.\n";
	return 0;
}
