#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace AGP::Tools::Detail
{
	inline constexpr std::array<std::byte, 8> StaticMeshMagic = {
		std::byte{ 'A' }, std::byte{ 'G' }, std::byte{ 'P' }, std::byte{ 'S' },
		std::byte{ 'M' }, std::byte{ 'E' }, std::byte{ 'S' }, std::byte{ 'H' }
	};

	inline constexpr std::uint32_t SectionCount = 3;
	inline constexpr std::uint32_t VertexSectionKind = 1;
	inline constexpr std::uint32_t IndexSectionKind = 2;
	inline constexpr std::uint32_t SubmeshSectionKind = 3;
	inline constexpr std::uint32_t VertexStride = 72;
	inline constexpr std::uint32_t IndexStride = 4;
	inline constexpr std::uint32_t SubmeshStride = 20;

	inline constexpr std::uint32_t FixedHeaderSize = 48;
	inline constexpr std::uint32_t SectionDescriptorSize = 24;
	inline constexpr std::uint32_t HeaderSize = FixedHeaderSize + SectionCount * SectionDescriptorSize;

	inline constexpr std::size_t SchemaVersionOffset = 8;
	inline constexpr std::size_t HeaderSizeOffset = 12;
	inline constexpr std::size_t FileSizeOffset = 16;
	inline constexpr std::size_t SectionCountOffset = 24;
	inline constexpr std::size_t VertexCountOffset = 28;
	inline constexpr std::size_t IndexCountOffset = 32;
	inline constexpr std::size_t SubmeshCountOffset = 36;
	inline constexpr std::size_t ReservedOffset = 40;
	inline constexpr std::size_t SectionDescriptorsOffset = FixedHeaderSize;
}
