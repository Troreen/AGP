#include "StaticMeshArtifact.h"
#include "StaticMeshArtifactFormat.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <span>
#include <system_error>
#include <type_traits>

namespace AGP::Tools
{
	namespace
	{
		using Bytes = std::vector<std::byte>;
		using ConstBytes = std::span<const std::byte>;

		constexpr std::uint32_t MaxVertexCount = 10'000'000;
		constexpr std::uint32_t MaxIndexCount = 30'000'000;
		constexpr std::uint32_t MaxSubmeshCount = 1'000'000;
		constexpr std::uint32_t MaxMaterialIndex = 65'535;
		constexpr std::uint64_t MaxArtifactSize = 2ull * 1024ull * 1024ull * 1024ull;

		struct SectionDescriptor
		{
			std::uint32_t Kind = 0;
			std::uint32_t Stride = 0;
			std::uint64_t Offset = 0;
			std::uint64_t Size = 0;
		};

		struct ParsedHeader
		{
			std::uint64_t FileSize = 0;
			std::uint32_t VertexCount = 0;
			std::uint32_t IndexCount = 0;
			std::uint32_t SubmeshCount = 0;
			std::array<SectionDescriptor, Detail::SectionCount> Sections;
		};

		template <typename T>
		struct LittleEndianStorage
		{
			using Type = std::make_unsigned_t<T>;
		};

		template <>
		struct LittleEndianStorage<float>
		{
			using Type = std::uint32_t;
		};

		ArtifactValidationResult Failure(
			std::string aCode,
			std::string aMessage,
			const std::uint64_t aByteOffset = 0)
		{
			ArtifactValidationResult result;
			result.Diagnostics.push_back({
				ArtifactDiagnosticSeverity::Error,
				std::move(aCode),
				std::move(aMessage),
				aByteOffset
			});
			return result;
		}

		void AttachSource(ArtifactValidationResult& aResult, const std::filesystem::path& aSourcePath)
		{
			for (ArtifactDiagnostic& diagnostic : aResult.Diagnostics)
			{
				if (diagnostic.SourcePath.empty()) diagnostic.SourcePath = aSourcePath;
			}
		}

		template <typename T>
		void AppendLittleEndian(Bytes& someBytes, const T aValue)
		{
			static_assert(std::is_integral_v<T> || std::is_same_v<T, float>);
			using Storage = typename LittleEndianStorage<T>::Type;
			const Storage value = [&]
			{
				if constexpr (std::is_same_v<T, float>)
				{
					return std::bit_cast<std::uint32_t>(aValue);
				}
				else
				{
					return static_cast<Storage>(aValue);
				}
			}();

			for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex)
			{
				someBytes.push_back(static_cast<std::byte>((value >> (byteIndex * 8)) & 0xffu));
			}
		}

		template <typename T>
		bool ReadLittleEndian(const ConstBytes someBytes, const std::size_t anOffset, T& outValue)
		{
			static_assert(std::is_integral_v<T> || std::is_same_v<T, float>);
			using Storage = typename LittleEndianStorage<T>::Type;
			if (anOffset > someBytes.size() || sizeof(Storage) > someBytes.size() - anOffset)
			{
				return false;
			}

			Storage value = 0;
			for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex)
			{
				value |= static_cast<Storage>(std::to_integer<unsigned char>(someBytes[anOffset + byteIndex])) << (byteIndex * 8);
			}

			if constexpr (std::is_same_v<T, float>)
			{
				outValue = std::bit_cast<float>(value);
			}
			else
			{
				outValue = static_cast<T>(value);
			}
			return true;
		}

		std::optional<Bytes> ReadFile(const std::filesystem::path& aPath, ArtifactValidationResult& outResult)
		{
			std::error_code error;
			const std::uintmax_t fileSize = std::filesystem::file_size(aPath, error);
			if (error)
			{
				outResult = Failure("artifact.io.open_failed", "Could not inspect static-mesh artifact: " + error.message());
				return std::nullopt;
			}
			if (fileSize > MaxArtifactSize)
			{
				outResult = Failure("artifact.size.limit", "Static-mesh artifact exceeds the supported two-gigabyte limit.");
				return std::nullopt;
			}

			std::ifstream input(aPath, std::ios::binary);
			if (!input)
			{
				outResult = Failure("artifact.io.open_failed", "Could not open static-mesh artifact for reading.");
				return std::nullopt;
			}

			Bytes bytes(static_cast<std::size_t>(fileSize));
			if (!bytes.empty())
			{
				input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
			}
			if (!input || input.peek() != std::ifstream::traits_type::eof())
			{
				outResult = Failure("artifact.io.read_failed", "Could not read the complete static-mesh artifact.");
				return std::nullopt;
			}
			return bytes;
		}

		bool CheckedProduct(const std::uint32_t aCount, const std::uint32_t aStride, std::uint64_t& outProduct)
		{
			outProduct = static_cast<std::uint64_t>(aCount) * static_cast<std::uint64_t>(aStride);
			return aStride == 0 || outProduct / aStride == aCount;
		}

		ArtifactValidationResult ParseAndValidateHeader(const ConstBytes someBytes, ParsedHeader& outHeader)
		{
			if (someBytes.size() < Detail::HeaderSize)
			{
				return Failure("artifact.header.truncated", "Static-mesh artifact is smaller than its required header.");
			}
			if (!std::equal(Detail::StaticMeshMagic.begin(), Detail::StaticMeshMagic.end(), someBytes.begin()))
			{
				return Failure("artifact.header.magic", "Static-mesh artifact magic is invalid.");
			}

			std::uint32_t schemaVersion = 0;
			std::uint32_t headerSize = 0;
			std::uint32_t sectionCount = 0;
			std::uint64_t reserved = 0;
			ReadLittleEndian(someBytes, Detail::SchemaVersionOffset, schemaVersion);
			ReadLittleEndian(someBytes, Detail::HeaderSizeOffset, headerSize);
			ReadLittleEndian(someBytes, Detail::FileSizeOffset, outHeader.FileSize);
			ReadLittleEndian(someBytes, Detail::SectionCountOffset, sectionCount);
			ReadLittleEndian(someBytes, Detail::VertexCountOffset, outHeader.VertexCount);
			ReadLittleEndian(someBytes, Detail::IndexCountOffset, outHeader.IndexCount);
			ReadLittleEndian(someBytes, Detail::SubmeshCountOffset, outHeader.SubmeshCount);
			ReadLittleEndian(someBytes, Detail::ReservedOffset, reserved);

			if (schemaVersion != StaticMeshArtifactSchemaVersion)
			{
				return Failure("artifact.header.version", "Static-mesh artifact schema version is unsupported.", Detail::SchemaVersionOffset);
			}
			if (headerSize != Detail::HeaderSize)
			{
				return Failure("artifact.header.size", "Static-mesh artifact header size is invalid.", Detail::HeaderSizeOffset);
			}
			if (outHeader.FileSize != someBytes.size())
			{
				return Failure("artifact.file.size", "Declared artifact size does not match the complete file size.", Detail::FileSizeOffset);
			}
			if (sectionCount != Detail::SectionCount)
			{
				return Failure("artifact.section.count", "Static-mesh artifact must contain exactly three sections.", Detail::SectionCountOffset);
			}
			if (reserved != 0)
			{
				return Failure("artifact.header.reserved", "Reserved static-mesh header bytes must be zero.", Detail::ReservedOffset);
			}
			if (outHeader.VertexCount == 0 || outHeader.VertexCount > MaxVertexCount)
			{
				return Failure("artifact.vertex.count", "Static-mesh vertex count is empty or exceeds the supported limit.", Detail::VertexCountOffset);
			}
			if (outHeader.IndexCount == 0 || outHeader.IndexCount > MaxIndexCount || outHeader.IndexCount % 3 != 0)
			{
				return Failure("artifact.index.count", "Static-mesh index count must be a non-empty triangle list within the supported limit.", Detail::IndexCountOffset);
			}
			if (outHeader.SubmeshCount == 0 || outHeader.SubmeshCount > MaxSubmeshCount)
			{
				return Failure("artifact.submesh.count", "Static-mesh submesh count is empty or exceeds the supported limit.", Detail::SubmeshCountOffset);
			}

			const std::array expectedKinds = { Detail::VertexSectionKind, Detail::IndexSectionKind, Detail::SubmeshSectionKind };
			const std::array expectedStrides = { Detail::VertexStride, Detail::IndexStride, Detail::SubmeshStride };
			const std::array expectedCounts = { outHeader.VertexCount, outHeader.IndexCount, outHeader.SubmeshCount };
			std::uint64_t expectedOffset = Detail::HeaderSize;
			for (std::size_t sectionIndex = 0; sectionIndex < outHeader.Sections.size(); ++sectionIndex)
			{
				const std::size_t descriptorOffset = Detail::SectionDescriptorsOffset + sectionIndex * Detail::SectionDescriptorSize;
				SectionDescriptor& section = outHeader.Sections[sectionIndex];
				ReadLittleEndian(someBytes, descriptorOffset, section.Kind);
				ReadLittleEndian(someBytes, descriptorOffset + 4, section.Stride);
				ReadLittleEndian(someBytes, descriptorOffset + 8, section.Offset);
				ReadLittleEndian(someBytes, descriptorOffset + 16, section.Size);

				if (section.Kind != expectedKinds[sectionIndex] || section.Stride != expectedStrides[sectionIndex])
				{
					return Failure("artifact.section.layout", "Static-mesh section kind or stride is invalid.", descriptorOffset);
				}
				std::uint64_t expectedSize = 0;
				if (!CheckedProduct(expectedCounts[sectionIndex], expectedStrides[sectionIndex], expectedSize) || section.Size != expectedSize)
				{
					return Failure("artifact.section.size", "Static-mesh section size does not match its count and stride.", descriptorOffset + 16);
				}
				if (section.Offset != expectedOffset)
				{
					return Failure("artifact.section.range", "Static-mesh sections must be ordered, contiguous, and non-overlapping.", descriptorOffset + 8);
				}
				if (section.Offset > outHeader.FileSize || section.Size > outHeader.FileSize - section.Offset)
				{
					return Failure("artifact.section.range", "Static-mesh section extends beyond the declared file.", descriptorOffset + 16);
				}
				expectedOffset += section.Size;
			}
			if (expectedOffset != outHeader.FileSize)
			{
				return Failure("artifact.file.trailing_data", "Static-mesh artifact contains unconsumed bytes.", expectedOffset);
			}
			return {};
		}

		ArtifactValidationResult ValidateBytes(const ConstBytes someBytes, ParsedHeader* outParsedHeader = nullptr)
		{
			ParsedHeader header;
			ArtifactValidationResult result = ParseAndValidateHeader(someBytes, header);
			if (!result.Succeeded())
			{
				return result;
			}

			const SectionDescriptor& vertices = header.Sections[0];
			for (std::uint32_t vertexIndex = 0; vertexIndex < header.VertexCount; ++vertexIndex)
			{
				const std::uint64_t vertexOffset = vertices.Offset + static_cast<std::uint64_t>(vertexIndex) * vertices.Stride;
				for (std::uint32_t component = 0; component < Detail::VertexStride / sizeof(float); ++component)
				{
					float value = 0.0f;
					ReadLittleEndian(someBytes, static_cast<std::size_t>(vertexOffset + component * sizeof(float)), value);
					if (!std::isfinite(value))
					{
						return Failure("artifact.vertex.non_finite", "Static-mesh vertex contains a non-finite component.", vertexOffset + component * sizeof(float));
					}
				}
			}

			const SectionDescriptor& indices = header.Sections[1];
			for (std::uint32_t index = 0; index < header.IndexCount; ++index)
			{
				std::uint32_t vertexIndex = 0;
				const std::uint64_t indexOffset = indices.Offset + static_cast<std::uint64_t>(index) * indices.Stride;
				ReadLittleEndian(someBytes, static_cast<std::size_t>(indexOffset), vertexIndex);
				if (vertexIndex >= header.VertexCount)
				{
					return Failure("artifact.index.range", "Static-mesh index refers outside the vertex section.", indexOffset);
				}
			}

			const SectionDescriptor& submeshes = header.Sections[2];
			for (std::uint32_t submeshIndex = 0; submeshIndex < header.SubmeshCount; ++submeshIndex)
			{
				const std::uint64_t submeshOffset = submeshes.Offset + static_cast<std::uint64_t>(submeshIndex) * submeshes.Stride;
				StaticMeshSubmesh submesh;
				ReadLittleEndian(someBytes, static_cast<std::size_t>(submeshOffset), submesh.VertexOffset);
				ReadLittleEndian(someBytes, static_cast<std::size_t>(submeshOffset + 4), submesh.IndexOffset);
				ReadLittleEndian(someBytes, static_cast<std::size_t>(submeshOffset + 8), submesh.VertexCount);
				ReadLittleEndian(someBytes, static_cast<std::size_t>(submeshOffset + 12), submesh.IndexCount);
				ReadLittleEndian(someBytes, static_cast<std::size_t>(submeshOffset + 16), submesh.MaterialIndex);

				if (submesh.VertexCount == 0 || submesh.VertexOffset > header.VertexCount || submesh.VertexCount > header.VertexCount - submesh.VertexOffset)
				{
					return Failure("artifact.submesh.vertex_range", "Static-mesh submesh vertex range is empty or out of bounds.", submeshOffset);
				}
				if (submesh.IndexCount == 0 || submesh.IndexCount % 3 != 0 || submesh.IndexOffset > header.IndexCount || submesh.IndexCount > header.IndexCount - submesh.IndexOffset)
				{
					return Failure("artifact.submesh.index_range", "Static-mesh submesh index range is not a valid in-bounds triangle list.", submeshOffset + 4);
				}
				if (submesh.MaterialIndex > MaxMaterialIndex)
				{
					return Failure("artifact.submesh.material_range", "Static-mesh material index exceeds the supported limit.", submeshOffset + 16);
				}

				const std::uint64_t vertexEnd = static_cast<std::uint64_t>(submesh.VertexOffset) + submesh.VertexCount;
				for (std::uint32_t localIndex = 0; localIndex < submesh.IndexCount; ++localIndex)
				{
					std::uint32_t vertexIndex = 0;
					const std::uint64_t indexOffset = indices.Offset + static_cast<std::uint64_t>(submesh.IndexOffset + localIndex) * indices.Stride;
					ReadLittleEndian(someBytes, static_cast<std::size_t>(indexOffset), vertexIndex);
					if (vertexIndex < submesh.VertexOffset || vertexIndex >= vertexEnd)
					{
						return Failure("artifact.submesh.index_vertex_range", "Static-mesh submesh index refers outside its declared vertex range.", indexOffset);
					}
				}
			}

			if (outParsedHeader != nullptr)
			{
				*outParsedHeader = header;
			}
			return result;
		}

		void AppendVertex(Bytes& someBytes, const StaticMeshVertex& aVertex)
		{
			const auto append = [&someBytes](const auto& someValues)
			{
				for (const float value : someValues)
				{
					AppendLittleEndian(someBytes, value);
				}
			};
			append(aVertex.Position);
			append(aVertex.Color);
			append(aVertex.UV0);
			append(aVertex.UV1);
			append(aVertex.Normal);
			append(aVertex.Tangent);
		}

		StaticMeshVertex ReadVertex(const ConstBytes someBytes, std::size_t& anOffset)
		{
			StaticMeshVertex vertex;
			const auto read = [&someBytes, &anOffset](auto& someValues)
			{
				for (float& value : someValues)
				{
					ReadLittleEndian(someBytes, anOffset, value);
					anOffset += sizeof(float);
				}
			};
			read(vertex.Position);
			read(vertex.Color);
			read(vertex.UV0);
			read(vertex.UV1);
			read(vertex.Normal);
			read(vertex.Tangent);
			return vertex;
		}
	}

	bool ArtifactValidationResult::Succeeded() const
	{
		return std::none_of(Diagnostics.begin(), Diagnostics.end(), [](const ArtifactDiagnostic& aDiagnostic)
		{
			return aDiagnostic.Severity == ArtifactDiagnosticSeverity::Error;
		});
	}

	bool StaticMeshReadResult::Succeeded() const
	{
		return Mesh.has_value() && std::none_of(Diagnostics.begin(), Diagnostics.end(), [](const ArtifactDiagnostic& aDiagnostic)
		{
			return aDiagnostic.Severity == ArtifactDiagnosticSeverity::Error;
		});
	}

	std::string_view GetStaticMeshToolVersion()
	{
		return StaticMeshToolVersion;
	}

	ArtifactValidationResult ValidateStaticMeshArtifact(const std::filesystem::path& anArtifactPath)
	{
		ArtifactValidationResult result;
		const std::optional<Bytes> bytes = ReadFile(anArtifactPath, result);
		if (!bytes.has_value())
		{
			AttachSource(result, anArtifactPath);
			return result;
		}
		result = ValidateBytes(*bytes);
		AttachSource(result, anArtifactPath);
		return result;
	}

	ArtifactValidationResult WriteStaticMeshArtifact(
		const std::filesystem::path& anArtifactPath,
		const StaticMeshData& aMesh)
	{
		if (aMesh.Vertices.size() > std::numeric_limits<std::uint32_t>::max()
			|| aMesh.Indices.size() > std::numeric_limits<std::uint32_t>::max()
			|| aMesh.Submeshes.size() > std::numeric_limits<std::uint32_t>::max())
		{
			return Failure("artifact.input.count", "Static-mesh input contains more elements than the artifact format can represent.");
		}

		const std::uint32_t vertexCount = static_cast<std::uint32_t>(aMesh.Vertices.size());
		const std::uint32_t indexCount = static_cast<std::uint32_t>(aMesh.Indices.size());
		const std::uint32_t submeshCount = static_cast<std::uint32_t>(aMesh.Submeshes.size());
		const std::uint64_t vertexSize = static_cast<std::uint64_t>(vertexCount) * Detail::VertexStride;
		const std::uint64_t indexSize = static_cast<std::uint64_t>(indexCount) * Detail::IndexStride;
		const std::uint64_t submeshSize = static_cast<std::uint64_t>(submeshCount) * Detail::SubmeshStride;
		const std::uint64_t fileSize = Detail::HeaderSize + vertexSize + indexSize + submeshSize;
		if (fileSize > MaxArtifactSize)
		{
			return Failure("artifact.size.limit", "Static-mesh output would exceed the supported two-gigabyte limit.");
		}

		Bytes bytes;
		bytes.reserve(static_cast<std::size_t>(fileSize));
		bytes.insert(bytes.end(), Detail::StaticMeshMagic.begin(), Detail::StaticMeshMagic.end());
		AppendLittleEndian(bytes, StaticMeshArtifactSchemaVersion);
		AppendLittleEndian(bytes, Detail::HeaderSize);
		AppendLittleEndian(bytes, fileSize);
		AppendLittleEndian(bytes, Detail::SectionCount);
		AppendLittleEndian(bytes, vertexCount);
		AppendLittleEndian(bytes, indexCount);
		AppendLittleEndian(bytes, submeshCount);
		AppendLittleEndian(bytes, std::uint64_t{ 0 });

		std::uint64_t sectionOffset = Detail::HeaderSize;
		const auto appendSection = [&bytes, &sectionOffset](const std::uint32_t aKind, const std::uint32_t aStride, const std::uint64_t aSize)
		{
			AppendLittleEndian(bytes, aKind);
			AppendLittleEndian(bytes, aStride);
			AppendLittleEndian(bytes, sectionOffset);
			AppendLittleEndian(bytes, aSize);
			sectionOffset += aSize;
		};
		appendSection(Detail::VertexSectionKind, Detail::VertexStride, vertexSize);
		appendSection(Detail::IndexSectionKind, Detail::IndexStride, indexSize);
		appendSection(Detail::SubmeshSectionKind, Detail::SubmeshStride, submeshSize);

		for (const StaticMeshVertex& vertex : aMesh.Vertices)
		{
			AppendVertex(bytes, vertex);
		}
		for (const std::uint32_t index : aMesh.Indices)
		{
			AppendLittleEndian(bytes, index);
		}
		for (const StaticMeshSubmesh& submesh : aMesh.Submeshes)
		{
			AppendLittleEndian(bytes, submesh.VertexOffset);
			AppendLittleEndian(bytes, submesh.IndexOffset);
			AppendLittleEndian(bytes, submesh.VertexCount);
			AppendLittleEndian(bytes, submesh.IndexCount);
			AppendLittleEndian(bytes, submesh.MaterialIndex);
		}

		ArtifactValidationResult result = ValidateBytes(bytes);
		if (!result.Succeeded())
		{
			return result;
		}

		std::ofstream output(anArtifactPath, std::ios::binary | std::ios::trunc);
		if (!output)
		{
			return Failure("artifact.io.open_failed", "Could not open static-mesh artifact for writing.");
		}
		output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		output.close();
		if (!output)
		{
			return Failure("artifact.io.write_failed", "Could not write the complete static-mesh artifact.");
		}

		return ValidateStaticMeshArtifact(anArtifactPath);
	}

	StaticMeshReadResult ReadStaticMeshArtifact(const std::filesystem::path& anArtifactPath)
	{
		ArtifactValidationResult validation;
		const std::optional<Bytes> bytes = ReadFile(anArtifactPath, validation);
		if (!bytes.has_value())
		{
			AttachSource(validation, anArtifactPath);
			return { std::nullopt, std::move(validation.Diagnostics) };
		}

		ParsedHeader header;
		validation = ValidateBytes(*bytes, &header);
		if (!validation.Succeeded())
		{
			AttachSource(validation, anArtifactPath);
			return { std::nullopt, std::move(validation.Diagnostics) };
		}

		StaticMeshData mesh;
		mesh.Vertices.reserve(header.VertexCount);
		mesh.Indices.reserve(header.IndexCount);
		mesh.Submeshes.reserve(header.SubmeshCount);

		std::size_t offset = static_cast<std::size_t>(header.Sections[0].Offset);
		for (std::uint32_t vertexIndex = 0; vertexIndex < header.VertexCount; ++vertexIndex)
		{
			mesh.Vertices.push_back(ReadVertex(*bytes, offset));
		}
		offset = static_cast<std::size_t>(header.Sections[1].Offset);
		for (std::uint32_t index = 0; index < header.IndexCount; ++index)
		{
			std::uint32_t vertexIndex = 0;
			ReadLittleEndian(*bytes, offset, vertexIndex);
			offset += sizeof(vertexIndex);
			mesh.Indices.push_back(vertexIndex);
		}
		offset = static_cast<std::size_t>(header.Sections[2].Offset);
		for (std::uint32_t submeshIndex = 0; submeshIndex < header.SubmeshCount; ++submeshIndex)
		{
			StaticMeshSubmesh submesh;
			ReadLittleEndian(*bytes, offset, submesh.VertexOffset);
			ReadLittleEndian(*bytes, offset + 4, submesh.IndexOffset);
			ReadLittleEndian(*bytes, offset + 8, submesh.VertexCount);
			ReadLittleEndian(*bytes, offset + 12, submesh.IndexCount);
			ReadLittleEndian(*bytes, offset + 16, submesh.MaterialIndex);
			offset += Detail::SubmeshStride;
			mesh.Submeshes.push_back(submesh);
		}

		return { std::move(mesh), std::move(validation.Diagnostics) };
	}
}
