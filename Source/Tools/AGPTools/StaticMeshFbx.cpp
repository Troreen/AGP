#include "StaticMeshFbx.h"
#include "StaticMeshFbxInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <limits>
#include <mutex>
#include <string>

namespace AGP::Tools
{
	namespace
	{
		constexpr std::uint32_t MaxMaterialIndex = 65'535;
		constexpr float DirectionLengthEpsilon = 0.000001f;

		std::mutex& ImporterMutex()
		{
			static std::mutex mutex;
			return mutex;
		}

		class ImporterRuntime
		{
		public:
			ImporterRuntime()
			{
				TGA::FBX::Importer::InitImporter();
			}

			~ImporterRuntime()
			{
				TGA::FBX::Importer::UninitImporter();
			}
		};

		void EnsureImporterRuntime()
		{
			static ImporterRuntime runtime;
			(void)runtime;
		}

		ArtifactDiagnostic Diagnostic(
			const ArtifactDiagnosticSeverity aSeverity,
			std::string aCode,
			std::string aMessage,
			const std::filesystem::path& aSourcePath)
		{
			ArtifactDiagnostic diagnostic;
			diagnostic.Severity = aSeverity;
			diagnostic.Code = std::move(aCode);
			diagnostic.Message = std::move(aMessage);
			diagnostic.SourcePath = aSourcePath;
			return diagnostic;
		}

		StaticMeshConversionResult Failure(
			std::string aCode,
			std::string aMessage,
			const std::filesystem::path& aSourcePath)
		{
			StaticMeshConversionResult result;
			result.Diagnostics.push_back(Diagnostic(
				ArtifactDiagnosticSeverity::Error,
				std::move(aCode),
				std::move(aMessage),
				aSourcePath));
			return result;
		}

		StaticMeshConversionResult Canceled(const std::filesystem::path& aSourcePath)
		{
			StaticMeshConversionResult result;
			result.Canceled = true;
			result.Diagnostics.push_back(Diagnostic(
				ArtifactDiagnosticSeverity::Info,
				"tool.canceled",
				"Static-mesh FBX conversion was canceled.",
				aSourcePath));
			return result;
		}

		bool HasFbxExtension(const std::filesystem::path& aPath)
		{
			std::wstring extension = aPath.extension().wstring();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](const wchar_t aCharacter)
			{
				return static_cast<wchar_t>(std::towlower(aCharacter));
			});
			return extension == L".fbx";
		}

		bool IsFinite(const TGA::FBX::Vertex& aVertex)
		{
			for (const float value : aVertex.Position)
			{
				if (!std::isfinite(value)) return false;
			}
			for (const auto& color : aVertex.VertexColors)
			{
				for (const float value : color) if (!std::isfinite(value)) return false;
			}
			for (const auto& uv : aVertex.UVs)
			{
				for (const float value : uv) if (!std::isfinite(value)) return false;
			}
			for (const float value : aVertex.Normal) if (!std::isfinite(value)) return false;
			for (const float value : aVertex.Tangent) if (!std::isfinite(value)) return false;
			for (const float value : aVertex.BoneWeights) if (!std::isfinite(value)) return false;
			return true;
		}

		std::array<float, 3> ConvertDirection(
			const float* aSourceDirection,
			const std::array<float, 3>& aFallback)
		{
			const float lengthSquared =
				aSourceDirection[0] * aSourceDirection[0]
				+ aSourceDirection[1] * aSourceDirection[1]
				+ aSourceDirection[2] * aSourceDirection[2];
			if (lengthSquared <= DirectionLengthEpsilon)
			{
				return aFallback;
			}

			const float inverseLength = 1.0f / std::sqrt(lengthSquared);
			return {
				aSourceDirection[0] * inverseLength,
				aSourceDirection[1] * inverseLength,
				aSourceDirection[2] * inverseLength
			};
		}

		StaticMeshVertex ConvertVertex(const TGA::FBX::Vertex& aSourceVertex)
		{
			StaticMeshVertex vertex;
			std::copy_n(aSourceVertex.Position, vertex.Position.size(), vertex.Position.begin());
			if (aSourceVertex.VertexColors[0][3] > 0.0f)
			{
				std::copy_n(aSourceVertex.VertexColors[0], vertex.Color.size(), vertex.Color.begin());
			}
			std::copy_n(aSourceVertex.UVs[0], vertex.UV0.size(), vertex.UV0.begin());
			std::copy_n(aSourceVertex.UVs[1], vertex.UV1.size(), vertex.UV1.begin());
			vertex.Normal = ConvertDirection(aSourceVertex.Normal, { 0.0f, 0.0f, 1.0f });
			vertex.Tangent = ConvertDirection(aSourceVertex.Tangent, { 1.0f, 0.0f, 0.0f });
			return vertex;
		}

		bool HasSkinning(const TGA::FBX::Vertex& aVertex)
		{
			for (const float weight : aVertex.BoneWeights)
			{
				if (weight != 0.0f) return true;
			}
			return false;
		}

		void AttachSource(std::vector<ArtifactDiagnostic>& someDiagnostics, const std::filesystem::path& aSourcePath)
		{
			for (ArtifactDiagnostic& diagnostic : someDiagnostics)
			{
				if (diagnostic.SourcePath.empty()) diagnostic.SourcePath = aSourcePath;
			}
		}
	}

	bool StaticMeshConversionResult::Succeeded() const
	{
		return !Canceled && Mesh.has_value() && std::none_of(Diagnostics.begin(), Diagnostics.end(), [](const ArtifactDiagnostic& aDiagnostic)
		{
			return aDiagnostic.Severity == ArtifactDiagnosticSeverity::Error;
		});
	}

	bool StaticMeshArtifactBuildResult::Succeeded() const
	{
		return Completed && !Canceled && std::none_of(Diagnostics.begin(), Diagnostics.end(), [](const ArtifactDiagnostic& aDiagnostic)
		{
			return aDiagnostic.Severity == ArtifactDiagnosticSeverity::Error;
		});
	}

	namespace Detail
	{
		StaticMeshConversionResult ConvertImportedStaticMesh(
			const TGA::FBX::Mesh& anImportedMesh,
			const std::filesystem::path& aSourcePath)
		{
			if (!anImportedMesh.Skeleton.Bones.empty())
			{
				return Failure("fbx.mesh.skeletal_unsupported", "Skeletal FBX meshes are not supported by the static-mesh tool.", aSourcePath);
			}
			if (!anImportedMesh.LODGroups.empty())
			{
				return Failure("fbx.mesh.lod_unsupported", "FBX LOD groups are not supported by the static-mesh tool.", aSourcePath);
			}
			if (anImportedMesh.Elements.empty())
			{
				return Failure("fbx.mesh.empty", "FBX contains no renderable static-mesh elements.", aSourcePath);
			}

			StaticMeshData mesh;
			for (const TGA::FBX::Mesh::Element& sourceElement : anImportedMesh.Elements)
			{
				if (sourceElement.Vertices.empty() || sourceElement.Indices.empty())
				{
					return Failure("fbx.element.empty", "FBX contains an empty mesh element.", aSourcePath);
				}
				if (sourceElement.Indices.size() % 3 != 0)
				{
					return Failure("fbx.element.not_triangles", "FBX mesh element indices are not a triangle list.", aSourcePath);
				}
				if (sourceElement.MaterialIndex > MaxMaterialIndex)
				{
					return Failure("fbx.element.material_range", "FBX mesh element material index exceeds the supported range.", aSourcePath);
				}
				if (sourceElement.Vertices.size() > std::numeric_limits<std::uint32_t>::max() - mesh.Vertices.size()
					|| sourceElement.Indices.size() > std::numeric_limits<std::uint32_t>::max() - mesh.Indices.size())
				{
					return Failure("fbx.mesh.count_limit", "FBX mesh exceeds the artifact's 32-bit element-count range.", aSourcePath);
				}

				StaticMeshSubmesh submesh;
				submesh.VertexOffset = static_cast<std::uint32_t>(mesh.Vertices.size());
				submesh.IndexOffset = static_cast<std::uint32_t>(mesh.Indices.size());
				submesh.VertexCount = static_cast<std::uint32_t>(sourceElement.Vertices.size());
				submesh.IndexCount = static_cast<std::uint32_t>(sourceElement.Indices.size());
				submesh.MaterialIndex = sourceElement.MaterialIndex;

				mesh.Vertices.reserve(mesh.Vertices.size() + sourceElement.Vertices.size());
				for (const TGA::FBX::Vertex& sourceVertex : sourceElement.Vertices)
				{
					if (!IsFinite(sourceVertex))
					{
						return Failure("fbx.vertex.non_finite", "FBX mesh contains a non-finite vertex component.", aSourcePath);
					}
					if (HasSkinning(sourceVertex))
					{
						return Failure("fbx.mesh.skinning_unsupported", "Skinned vertices are not supported by the static-mesh tool.", aSourcePath);
					}
					mesh.Vertices.push_back(ConvertVertex(sourceVertex));
				}

				mesh.Indices.reserve(mesh.Indices.size() + sourceElement.Indices.size());
				for (const std::uint32_t sourceIndex : sourceElement.Indices)
				{
					if (sourceIndex >= sourceElement.Vertices.size())
					{
						return Failure("fbx.element.index_range", "FBX mesh element contains an out-of-range vertex index.", aSourcePath);
					}
					mesh.Indices.push_back(submesh.VertexOffset + sourceIndex);
				}
				mesh.Submeshes.push_back(submesh);
			}

			StaticMeshConversionResult result;
			result.Mesh = std::move(mesh);
			return result;
		}

		StaticMeshConversionResult ConvertFbxToStaticMeshDataWithLoader(
			const std::filesystem::path& aSourcePath,
			const std::atomic_bool& aCancellationRequested,
			const FbxMeshLoadFunction& aLoadMesh)
		{
			if (aCancellationRequested.load(std::memory_order_relaxed))
			{
				return Canceled(aSourcePath);
			}

			std::error_code error;
			if (!std::filesystem::is_regular_file(aSourcePath, error))
			{
				return Failure("fbx.source.missing", "FBX source is missing or is not a regular file.", aSourcePath);
			}
			if (!HasFbxExtension(aSourcePath))
			{
				return Failure("fbx.source.extension", "Static-mesh source must use the .fbx extension.", aSourcePath);
			}

			TGA::FBX::Mesh importedMesh;
			std::string importerError;
			bool loaded = false;
			try
			{
				loaded = aLoadMesh(aSourcePath, importedMesh, importerError);
			}
			catch (const std::exception& exception)
			{
				return Failure("fbx.import.exception", "FBX importer rejected the source: " + std::string(exception.what()), aSourcePath);
			}
			catch (...)
			{
				return Failure("fbx.import.exception", "FBX importer rejected the source with an unknown error.", aSourcePath);
			}

			if (aCancellationRequested.load(std::memory_order_relaxed))
			{
				return Canceled(aSourcePath);
			}
			if (!loaded)
			{
				if (importerError.empty()) importerError = "The FBX importer did not provide additional details.";
				return Failure("fbx.import.failed", "Could not import FBX: " + importerError, aSourcePath);
			}
			return ConvertImportedStaticMesh(importedMesh, aSourcePath);
		}
	}

	StaticMeshConversionResult ConvertFbxToStaticMeshData(
		const std::filesystem::path& aSourcePath,
		const std::atomic_bool& aCancellationRequested)
	{
		if (aCancellationRequested.load(std::memory_order_relaxed))
		{
			return Canceled(aSourcePath);
		}

		std::scoped_lock lock(ImporterMutex());
		EnsureImporterRuntime();
		return Detail::ConvertFbxToStaticMeshDataWithLoader(
			aSourcePath,
			aCancellationRequested,
			[](const std::filesystem::path& aPath, TGA::FBX::Mesh& outMesh, std::string& outError)
			{
				const bool loaded = TGA::FBX::Importer::LoadMeshW(aPath.wstring(), outMesh);
				if (!loaded) outError = TGA::FBX::Importer::GetLastError();
				return loaded;
			});
	}

	StaticMeshArtifactBuildResult BuildStaticMeshArtifactFromFbx(
		const std::filesystem::path& aSourcePath,
		const std::filesystem::path& anArtifactPath,
		const std::atomic_bool& aCancellationRequested)
	{
		StaticMeshConversionResult conversion = ConvertFbxToStaticMeshData(aSourcePath, aCancellationRequested);
		if (!conversion.Succeeded())
		{
			return { false, conversion.Canceled, std::move(conversion.Diagnostics) };
		}
		if (aCancellationRequested.load(std::memory_order_relaxed))
		{
			StaticMeshConversionResult canceled = Canceled(aSourcePath);
			return { false, true, std::move(canceled.Diagnostics) };
		}

		ArtifactValidationResult write = WriteStaticMeshArtifact(anArtifactPath, *conversion.Mesh);
		AttachSource(write.Diagnostics, anArtifactPath);
		if (!write.Succeeded())
		{
			return { false, false, std::move(write.Diagnostics) };
		}
		if (aCancellationRequested.load(std::memory_order_relaxed))
		{
			StaticMeshConversionResult canceled = Canceled(aSourcePath);
			return { false, true, std::move(canceled.Diagnostics) };
		}
		return { true, false, {} };
	}
}
