#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <TGAFbx.h>

namespace
{
	void WriteString(const std::string& aValue)
	{
		std::cout << '"';
		for (const unsigned char character : aValue)
		{
			if (character == '"' || character == '\\')
			{
				std::cout << '\\' << character;
			}
			else if (character < 32)
			{
				std::cout << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned>(character)
					<< std::dec << std::setfill(' ');
			}
			else
			{
				std::cout << character;
			}
		}
		std::cout << '"';
	}

	template <size_t Size>
	void WriteArray(const float (&aValues)[Size])
	{
		std::cout << '[';
		for (size_t index = 0; index < Size; ++index)
		{
			if (index != 0)
			{
				std::cout << ',';
			}
			if (std::isfinite(aValues[index]))
			{
				std::cout << aValues[index];
			}
			else
			{
				std::cout << "null";
			}
		}
		std::cout << ']';
	}

	void WriteVertex(size_t aIndex, const TGA::FBX::Vertex& aVertex)
	{
		std::cout << "{\"index\":" << aIndex << ",\"position\":";
		WriteArray(aVertex.Position);
		std::cout << ",\"uv0\":";
		WriteArray(aVertex.UVs[0]);
		std::cout << ",\"uv1\":";
		WriteArray(aVertex.UVs[1]);
		std::cout << ",\"normal\":";
		WriteArray(aVertex.Normal);
		std::cout << ",\"tangent\":";
		WriteArray(aVertex.Tangent);
		std::cout << ",\"binormal\":";
		WriteArray(aVertex.BiNormal);
		std::cout << '}';
	}

	void WriteElement(const TGA::FBX::Mesh::Element& aElement, bool aWriteAllVertices)
	{
		std::cout << "{\"name\":";
		WriteString(aElement.MeshName);
		std::cout << ",\"vertices\":" << aElement.Vertices.size() << ",\"indices\":" << aElement.Indices.size()
			<< ",\"materialIndex\":" << aElement.MaterialIndex << ",\"boundsMin\":";
		WriteArray(aElement.BoxBounds.Min);
		std::cout << ",\"boundsMax\":";
		WriteArray(aElement.BoxBounds.Max);
		std::cout << ",\"localTransform\":";
		WriteArray(aElement.LocalTransform.Data);
		std::cout << ",\"globalTransform\":";
		WriteArray(aElement.GlobalTransform.Data);
		std::cout << ",\"geometricTransform\":";
		WriteArray(aElement.GeometricTransform.Data);

		// Include asymmetric UV targets so a mirror cannot hide behind a symmetric
		// bounding box. Position extrema also expose axis and unit conversion.
		std::set<size_t> sampleIndices;
		if (!aElement.Vertices.empty())
		{
			sampleIndices = {0, aElement.Vertices.size() / 3, aElement.Vertices.size() - 1};
			for (int axis = 0; axis < 3; ++axis)
			{
				for (const bool findMaximum : {false, true})
				{
					size_t selected = 0;
					for (size_t index = 1; index < aElement.Vertices.size(); ++index)
					{
						const float candidate = aElement.Vertices[index].Position[axis];
						const float current = aElement.Vertices[selected].Position[axis];
						if ((findMaximum && candidate > current) || (!findMaximum && candidate < current))
						{
							selected = index;
						}
					}
					sampleIndices.insert(selected);
				}
			}
			for (const auto target : {std::pair{0.137f, 0.731f}, std::pair{0.829f, 0.263f}})
			{
				float nearestDistance = std::numeric_limits<float>::max();
				size_t selected = 0;
				for (size_t index = 0; index < aElement.Vertices.size(); ++index)
				{
					const auto& uv = aElement.Vertices[index].UVs[0];
					const float distance = (uv[0] - target.first) * (uv[0] - target.first)
						+ (uv[1] - target.second) * (uv[1] - target.second);
					if (distance < nearestDistance)
					{
						nearestDistance = distance;
						selected = index;
					}
				}
				sampleIndices.insert(selected);
			}
		}
		std::cout << ",\"samples\":[";
		bool first = true;
		for (size_t index = 0; index < aElement.Vertices.size(); ++index)
		{
			if (!aWriteAllVertices && !sampleIndices.contains(index))
			{
				continue;
			}
			if (!first)
			{
				std::cout << ',';
			}
			first = false;
			WriteVertex(index, aElement.Vertices[index]);
		}
		std::cout << "]}";
	}
}

int wmain(int aArgumentCount, wchar_t** aArguments)
{
	if (aArgumentCount < 2 || aArgumentCount > 3)
	{
		std::cerr << "Usage: FbxImporterProbe.exe <mesh.fbx> [--all]\n";
		return 2;
	}
	const bool writeAllVertices = aArgumentCount == 3 && std::wstring(aArguments[2]) == L"--all";
	if (aArgumentCount == 3 && !writeAllVertices)
	{
		std::cerr << "Unknown option\n";
		return 2;
	}
	TGA::FBX::Importer::InitImporter();
	TGA::FBX::Mesh mesh{};
	const auto status = TGA::FBX::Importer::LoadMesh(std::filesystem::path(aArguments[1]), mesh);
	if (!status)
	{
		std::cerr << "LoadMesh failed: " << TGA::FBX::Importer::GetLastSDKError() << '\n';
		TGA::FBX::Importer::UninitImporter();
		return 1;
	}
	std::cout << std::setprecision(9);
	std::cout << "{\"status\":" << static_cast<unsigned>(status.Result) << ",\"originalUpAxis\":"
		<< static_cast<unsigned>(mesh.FileInfo.OriginalUpAxis) << ",\"originalSystemUnit\":"
		<< static_cast<unsigned>(mesh.FileInfo.OriginalSystemUnit) << ",\"totalVertices\":" << mesh.TotalVertexCount
		<< ",\"totalIndices\":" << mesh.TotalIndexCount << ",\"boundsMin\":";
	WriteArray(mesh.BoxBounds.Min);
	std::cout << ",\"boundsMax\":";
	WriteArray(mesh.BoxBounds.Max);
	std::cout << ",\"elements\":[";
	for (size_t index = 0; index < mesh.Elements.size(); ++index)
	{
		if (index != 0)
		{
			std::cout << ',';
		}
		WriteElement(mesh.Elements[index], writeAllVertices);
	}
	std::cout << "]}\n";
	TGA::FBX::Importer::UninitImporter();
	return 0;
}
