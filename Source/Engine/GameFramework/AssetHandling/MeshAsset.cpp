#include "GameFramework.pch.h"
#include "MeshAsset.h"

#include "AssetLoadingHelper.h"

#include <GraphicsEngine/Objects/Vertex.h>
#include <Importer.h>
#include <Logger/Logger.h>

DECLARE_LOG_CATEGORY_WITH_NAME(MeshAssetLog, ASSET, Verbose);

DEFINE_LOG_CATEGORY(MeshAssetLog);

MeshAsset::MeshAsset() = default;

MeshAsset::MeshAsset(const std::shared_ptr<Mesh>& aMesh) : myMesh(aMesh)
{
}

MeshAsset::~MeshAsset() = default;

const std::shared_ptr<Mesh>& MeshAsset::GetMesh() const
{
	return myMesh;
}

bool MeshAsset::Load(const std::filesystem::path& aPath, [[maybe_unused]] AssetRegistry& aRegistry)
{
	TGA::FBX::Mesh importedMesh;
	if (!TGA::FBX::Importer::LoadMesh(aPath.wstring(), importedMesh))
	{
		LOG(MeshAssetLog, Warning, "Could not load FBX mesh '{}': {}", aPath.string(), TGA::FBX::Importer::GetLastSDKError());
		return false;
	}

	if (!importedMesh.IsValid())
	{
		LOG(MeshAssetLog, Warning, "Could not load FBX mesh '{}': imported mesh was empty.", aPath.string());
		return false;
	}

	std::vector<Mesh::Element> elements;
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	const std::string meshName = aPath.stem().string();
	const bool isLandscape = meshName.starts_with("SM_Landscape_");

	if (!isLandscape)
	{
		SetElements(importedMesh, elements, vertices, indices);
	}
	else
	{
		SetLandscapeElements(importedMesh, elements, vertices, indices);
	}

	if (elements.empty())
	{
		LOG(MeshAssetLog, Warning, "Could not load FBX mesh '{}': no renderable mesh elements were found.", aPath.string());
		return false;
	}

	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
	mesh->Initialize(meshName, std::move(elements), std::move(vertices), std::move(indices));
	if (!importedMesh.Skeleton.Bones.empty())
	{
		mesh->SetSkeleton(AssetHelper::ConvertSkeleton(importedMesh.Skeleton));
	}

	myMesh = std::move(mesh);
	LOG(MeshAssetLog, Log, "Loaded FBX mesh '{}' with {} elements.", meshName, importedMesh.Elements.size());
	return true;
}

void MeshAsset::SetElements(const TGA::FBX::Mesh& aImportedMesh,
	std::vector<Mesh::Element>& inoutElements, std::vector<Vertex>& inoutVertices, std::vector<unsigned>& inoutIndices) const
{
	for (const TGA::FBX::Mesh::Element& importedElement : aImportedMesh.Elements)
	{
		if (importedElement.Vertices.empty() || importedElement.Indices.empty())
		{
			continue;
		}

		Mesh::Element element;
		element.VertexOffset = static_cast<unsigned>(inoutVertices.size());
		element.IndexOffset = static_cast<unsigned>(inoutIndices.size());
		element.NumVertices = static_cast<unsigned>(importedElement.Vertices.size());
		element.NumIndices = static_cast<unsigned>(importedElement.Indices.size());
		element.MaterialIndex = importedElement.MaterialIndex;

		inoutVertices.reserve(inoutVertices.size() + importedElement.Vertices.size());
		inoutIndices.reserve(inoutIndices.size() + importedElement.Indices.size());

		for (const TGA::FBX::Vertex& sourceVertex : importedElement.Vertices)
		{
			inoutVertices.push_back(AssetHelper::ConvertVertex(sourceVertex));
		}

		for (const unsigned sourceIndex : importedElement.Indices)
		{
			inoutIndices.push_back(element.VertexOffset + sourceIndex);
		}

		inoutElements.push_back(element);
	}
}

void MeshAsset::SetLandscapeElements(const TGA::FBX::Mesh& aImportedMesh,
	std::vector<Mesh::Element>& inoutElements, std::vector<Vertex>& inoutVertices, std::vector<unsigned>& inoutIndices) const
{
	for (const TGA::FBX::Mesh::Element& importedElement : aImportedMesh.Elements)
	{
		if (importedElement.Vertices.empty() || importedElement.Indices.empty())
		{
			continue;
		}

		Mesh::Element element;
		element.VertexOffset = static_cast<unsigned>(inoutVertices.size());
		element.IndexOffset = static_cast<unsigned>(inoutIndices.size());
		element.NumVertices = static_cast<unsigned>(importedElement.Vertices.size());
		element.NumIndices = static_cast<unsigned>(importedElement.Indices.size());
		element.MaterialIndex = importedElement.MaterialIndex;

		inoutVertices.reserve(inoutVertices.size() + importedElement.Vertices.size());
		inoutIndices.reserve(inoutIndices.size() + importedElement.Indices.size());

		for (const TGA::FBX::Vertex& sourceVertex : importedElement.Vertices)
		{
			inoutVertices.push_back(AssetHelper::ConvertVertex(sourceVertex));
			inoutVertices[inoutVertices.size() - 1].Normal = CU::Vector3f::Zero;
		}

		int triangleCounter = 0;
		for (size_t i = 0; i < importedElement.Indices.size(); ++i)
		{
			inoutIndices.push_back(element.VertexOffset + importedElement.Indices[i]);

			if (triangleCounter == 2)
			{
				const unsigned index1 = inoutIndices[i - 2];
				const unsigned index2 = inoutIndices[i - 1];
				const unsigned index3 = inoutIndices[i - 0];

				const CU::Vector3f vertex1Position = inoutVertices[index1].Position.XYZ();
				const CU::Vector3f vertex2Position = inoutVertices[index2].Position.XYZ();
				const CU::Vector3f vertex3Position = inoutVertices[index3].Position.XYZ();

				const CU::Vector3f a = vertex2Position - vertex1Position;
				const CU::Vector3f b = vertex3Position - vertex1Position;
				const CU::Vector3f faceNormal = a.Cross(b);

				inoutVertices[index1].Normal += faceNormal;
				inoutVertices[index2].Normal += faceNormal;
				inoutVertices[index3].Normal += faceNormal;
			}
			triangleCounter = (triangleCounter + 1) % 3;
		}

		inoutElements.push_back(element);
	}

	for (Vertex& vertex : inoutVertices)
	{
		vertex.Normal.Normalize();
	}
}
