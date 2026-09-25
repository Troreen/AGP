#include "MeshAsset.h"

#include "AssetLoadingHelper.h"

#include <Importer.h>
#include <GraphicsEngine/Objects/Vertex.h>
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

	for (const TGA::FBX::Mesh::Element& importedElement : importedMesh.Elements)
	{
		if (importedElement.Vertices.empty() || importedElement.Indices.empty())
		{
			continue;
		}

		Mesh::Element element;
		element.VertexOffset = static_cast<unsigned>(vertices.size());
		element.IndexOffset = static_cast<unsigned>(indices.size());
		element.NumVertices = static_cast<unsigned>(importedElement.Vertices.size());
		element.NumIndices = static_cast<unsigned>(importedElement.Indices.size());
		element.MaterialIndex = importedElement.MaterialIndex;

		vertices.reserve(vertices.size() + importedElement.Vertices.size());
		indices.reserve(indices.size() + importedElement.Indices.size());

		for (const TGA::FBX::Vertex& sourceVertex : importedElement.Vertices)
		{
			vertices.push_back(AssetHelper::ConvertVertex(sourceVertex));
		}

		for (const unsigned sourceIndex : importedElement.Indices)
		{
			indices.push_back(element.VertexOffset + sourceIndex);
		}

		elements.push_back(element);
	}

	if (elements.empty())
	{
		LOG(MeshAssetLog, Warning, "Could not load FBX mesh '{}': no renderable mesh elements were found.", aPath.string());
		return false;
	}

	const std::string meshName = aPath.stem().string();
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
