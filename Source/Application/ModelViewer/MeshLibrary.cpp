#include "MeshLibrary.h"

#include "Application.h"
#include "GraphicsEngine/Objects/Mesh.h"
#include "GraphicsEngine/Objects/Vertex.h"
#include "PrimitiveMeshBuilder.h"
#include "Importer.h"

#include <array>
#include <random>
#include <utility>
#include <vector>

namespace
{
	CommonUtilities::Vector4f CreateRandomColor(std::mt19937& aRandomGenerator)
	{
		std::uniform_real_distribution<float> colorDistribution(0.2f, 1.0f);
		return {
			colorDistribution(aRandomGenerator),
			colorDistribution(aRandomGenerator),
			colorDistribution(aRandomGenerator),
			1.0f
		};
	}

	bool HasImportedColor(const TGA::FBX::Vertex& aVertex)
	{
		return aVertex.VertexColors[0][3] > 0.0f;
	}

	Vertex ConvertVertex(const TGA::FBX::Vertex& aSourceVertex, const CommonUtilities::Vector4f& aFallbackColor)
	{
		Vertex vertex;
		vertex.Position = {
			aSourceVertex.Position[0],
			aSourceVertex.Position[1],
			aSourceVertex.Position[2],
			aSourceVertex.Position[3]
		};

		if (HasImportedColor(aSourceVertex))
		{
			vertex.Color = {
				aSourceVertex.VertexColors[0][0],
				aSourceVertex.VertexColors[0][1],
				aSourceVertex.VertexColors[0][2],
				aSourceVertex.VertexColors[0][3]
			};
		}
		else
		{
			vertex.Color = aFallbackColor;
		}

		return vertex;
	}

	void AppendElement(
		const TGA::FBX::Mesh::Element& aSourceElement,
		std::vector<Mesh::Element>& outElements,
		std::vector<Vertex>& outVertices,
		std::vector<unsigned>& outIndices,
		std::mt19937& aRandomGenerator)
	{
		if (aSourceElement.Vertices.empty() || aSourceElement.Indices.empty())
		{
			return;
		}

		Mesh::Element element;
		element.VertexOffset = static_cast<unsigned>(outVertices.size());
		element.IndexOffset = static_cast<unsigned>(outIndices.size());
		element.NumVertices = static_cast<unsigned>(aSourceElement.Vertices.size());
		element.NumIndices = static_cast<unsigned>(aSourceElement.Indices.size());
		element.MaterialIndex = aSourceElement.MaterialIndex;

		outVertices.reserve(outVertices.size() + aSourceElement.Vertices.size());
		outIndices.reserve(outIndices.size() + aSourceElement.Indices.size());

		for (const TGA::FBX::Vertex& sourceVertex : aSourceElement.Vertices)
		{
			outVertices.push_back(ConvertVertex(sourceVertex, CreateRandomColor(aRandomGenerator)));
		}

		for (const unsigned sourceIndex : aSourceElement.Indices)
		{
			outIndices.push_back(element.VertexOffset + sourceIndex);
		}

		outElements.push_back(element);
	}
}

MeshLibrary::MeshLibrary()
{
	TGA::FBX::Importer::InitImporter();
}

MeshLibrary::~MeshLibrary()
{
	TGA::FBX::Importer::UninitImporter();
}

void MeshLibrary::Initialize()
{
	RegisterPrimitiveMeshes();
	LoadFBXMesh("SM_Chest.fbx");
}

std::shared_ptr<Mesh> MeshLibrary::GetMesh(std::string_view aName) const
{
	const auto foundMesh = myMeshes.find(std::string(aName));
	if (foundMesh == myMeshes.end())
	{
		return nullptr;
	}

	return foundMesh->second;
}

bool MeshLibrary::LoadFBXMesh(const std::filesystem::path& aPath)
{
	const std::filesystem::path resolvedPath = ResolvePath(aPath);
	if (resolvedPath.empty())
	{
		MVLOG(Warning, "Could not load FBX mesh '{}': file was not found.", aPath.string());
		return false;
	}

	TGA::FBX::Mesh importedMesh;
	if (!TGA::FBX::Importer::LoadMeshW(resolvedPath.wstring(), importedMesh))
	{
		MVLOG(Warning, "Could not load FBX mesh '{}': {}", resolvedPath.string(), TGA::FBX::Importer::GetLastError());
		return false;
	}

	if (!importedMesh.IsValid())
	{
		MVLOG(Warning, "Could not load FBX mesh '{}': imported mesh was empty.", resolvedPath.string());
		return false;
	}

	std::vector<Mesh::Element> elements;
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	std::mt19937 randomGenerator(0x2A1FBCu);

	for (const TGA::FBX::Mesh::Element& importedElement : importedMesh.Elements)
	{
		AppendElement(importedElement, elements, vertices, indices, randomGenerator);
	}

	if (elements.empty())
	{
		MVLOG(Warning, "Could not load FBX mesh '{}': no renderable mesh elements were found.", resolvedPath.string());
		return false;
	}

	const std::string meshName = resolvedPath.stem().string();
	auto mesh = std::make_shared<Mesh>();
	mesh->Initialize(meshName, std::move(elements), std::move(vertices), std::move(indices));
	RegisterMesh(meshName, mesh);

	MVLOG(Log, "Loaded FBX mesh '{}' with {} elements.", meshName, importedMesh.Elements.size());
	return true;
}

void MeshLibrary::RegisterPrimitiveMeshes()
{
	RegisterMesh("Plane", PrimitiveMeshBuilder::CreatePlane());
	RegisterMesh("Cube", PrimitiveMeshBuilder::CreateCube());
	RegisterMesh("Pyramid", PrimitiveMeshBuilder::CreatePyramid());
	RegisterMesh("Sphere", PrimitiveMeshBuilder::CreateSphere());
	RegisterMesh("Torus", PrimitiveMeshBuilder::CreateTorus());
	RegisterMesh("WorldAxes", PrimitiveMeshBuilder::CreateAxes());
}

void MeshLibrary::RegisterMesh(std::string aName, std::shared_ptr<Mesh> aMesh)
{
	if (aName.empty() || aMesh == nullptr)
	{
		return;
	}

	myMeshes[std::move(aName)] = std::move(aMesh);
}

std::filesystem::path MeshLibrary::ResolvePath(const std::filesystem::path& aPath) const
{
	if (std::filesystem::exists(aPath))
	{
		return aPath;
	}

	std::filesystem::path basePath = std::filesystem::current_path();
	for (int depth = 0; depth < 5; ++depth)
	{
		const std::filesystem::path candidate = basePath / aPath;
		if (std::filesystem::exists(candidate))
		{
			return candidate;
		}

		basePath /= "..";
	}

	return {};
}
