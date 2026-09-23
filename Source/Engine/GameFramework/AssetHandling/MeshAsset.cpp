#include "MeshAsset.h"

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

bool MeshAsset::Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry)
{
	TGA::FBX::Mesh importedMesh;
	if (!TGA::FBX::Importer::LoadMeshW(aPath.wstring(), importedMesh))
	{
		LOG(MeshAssetLog, Warning, "Could not load FBX mesh '{}': {}", aPath.string(), TGA::FBX::Importer::GetLastError());
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
			vertices.push_back(ConvertVertex(sourceVertex));
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
		mesh->SetSkeleton(ConvertSkeleton(importedMesh.Skeleton));
	}

	myMesh = std::move(mesh);
	LOG(MeshAssetLog, Log, "Loaded FBX mesh '{}' with {} elements.", meshName, importedMesh.Elements.size());
	return true;
}

Vertex MeshAsset::ConvertVertex(const TGA::FBX::Vertex& aSourceVertex)
{
	const CommonUtilities::Vector4f DefaultVertexColor = { 1.0f, 1.0f, 1.0f, 1.0f };

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
		vertex.Color = DefaultVertexColor;
	}

	vertex.BoneIDs = {
		aSourceVertex.BoneIDs[0],
		aSourceVertex.BoneIDs[1],
		aSourceVertex.BoneIDs[2],
		aSourceVertex.BoneIDs[3]
	};

	vertex.SkinWeights = {
		aSourceVertex.BoneWeights[0],
		aSourceVertex.BoneWeights[1],
		aSourceVertex.BoneWeights[2],
		aSourceVertex.BoneWeights[3]
	};

	vertex.UV0 = {
		aSourceVertex.UVs[0][0],
		aSourceVertex.UVs[0][1]
	};

	vertex.UV1 = {
		aSourceVertex.UVs[1][0],
		aSourceVertex.UVs[1][1]
	};

	vertex.Normal = ConvertDirection(aSourceVertex.Normal, CommonUtilities::Vector3f::UnitZ);
	vertex.Tangent = ConvertDirection(aSourceVertex.Tangent, CommonUtilities::Vector3f::UnitX);

	const float totalWeight =
		vertex.SkinWeights.x +
		vertex.SkinWeights.y +
		vertex.SkinWeights.z +
		vertex.SkinWeights.w;
	if (totalWeight > 0.0f)
	{
		vertex.SkinWeights.x /= totalWeight;
		vertex.SkinWeights.y /= totalWeight;
		vertex.SkinWeights.z /= totalWeight;
		vertex.SkinWeights.w /= totalWeight;
	}

	return vertex;
}

bool MeshAsset::HasImportedColor(const TGA::FBX::Vertex& aVertex)
{
	return aVertex.VertexColors[0][3] > 0.0f;
}

CU::Vector3f MeshAsset::ConvertDirection(const float* aSourceVector, const CU::Vector3f& aFallback)
{
	CommonUtilities::Vector3f direction(
		aSourceVector[0],
		aSourceVector[1],
		aSourceVector[2]);

	if (direction.LengthSqr() <= 0.000001f)
	{
		return aFallback;
	}

	return direction.GetNormalized();
}

Skeleton MeshAsset::ConvertSkeleton(const TGA::FBX::Skeleton& aSourceSkeleton)
{
	Skeleton skeleton;
	skeleton.Joints.reserve(aSourceSkeleton.Bones.size());

	for (const TGA::FBX::Skeleton::Bone& sourceBone : aSourceSkeleton.Bones)
	{
		Skeleton::Joint joint;
		joint.BindPoseInverse = ConvertMatrix(sourceBone.BindPoseInverse).GetTranspose();
		joint.Parent = sourceBone.ParentIdx;
		joint.Name = sourceBone.Name;
		joint.Children.reserve(sourceBone.Children.size());

		for (const unsigned childIndex : sourceBone.Children)
		{
			joint.Children.push_back(static_cast<int>(childIndex));
		}

		skeleton.Joints.push_back(std::move(joint));
	}

	for (size_t jointIndex = 0; jointIndex < skeleton.Joints.size(); ++jointIndex)
	{
		skeleton.JointNameToIndex[skeleton.Joints[jointIndex].Name] = jointIndex;
	}

	return skeleton;
}

CU::Matrix4f MeshAsset::ConvertMatrix(const TGA::FBX::Matrix& aSourceMatrix)
{
	return {
		aSourceMatrix(1, 1), aSourceMatrix(1, 2), aSourceMatrix(1, 3), aSourceMatrix(1, 4),
		aSourceMatrix(2, 1), aSourceMatrix(2, 2), aSourceMatrix(2, 3), aSourceMatrix(2, 4),
		aSourceMatrix(3, 1), aSourceMatrix(3, 2), aSourceMatrix(3, 3), aSourceMatrix(3, 4),
		aSourceMatrix(4, 1), aSourceMatrix(4, 2), aSourceMatrix(4, 3), aSourceMatrix(4, 4)
	};
}
