#include "MeshLibrary.h"

#include "Application.h"
#include "GraphicsEngine/Objects/Mesh.h"
#include "GraphicsEngine/Objects/Vertex.h"
#include "PrimitiveMeshBuilder.h"
#include "Importer.h"

#include <utility>
#include <vector>

namespace
{
	const CommonUtilities::Vector4f DefaultVertexColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	bool HasImportedColor(const TGA::FBX::Vertex& aVertex)
	{
		return aVertex.VertexColors[0][3] > 0.0f;
	}

	CommonUtilities::Matrix4f ConvertMatrix(const TGA::FBX::Matrix& aSourceMatrix)
	{
		return {
			aSourceMatrix(1, 1), aSourceMatrix(1, 2), aSourceMatrix(1, 3), aSourceMatrix(1, 4),
			aSourceMatrix(2, 1), aSourceMatrix(2, 2), aSourceMatrix(2, 3), aSourceMatrix(2, 4),
			aSourceMatrix(3, 1), aSourceMatrix(3, 2), aSourceMatrix(3, 3), aSourceMatrix(3, 4),
			aSourceMatrix(4, 1), aSourceMatrix(4, 2), aSourceMatrix(4, 3), aSourceMatrix(4, 4)
		};
	}

	CommonUtilities::Vector3f ConvertDirection(const float* aSourceVector, const CommonUtilities::Vector3f& aFallback)
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

	Skeleton ConvertSkeleton(const TGA::FBX::Skeleton& aSourceSkeleton)
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

	std::shared_ptr<Animation> ConvertAnimation(const TGA::FBX::Animation& aSourceAnimation, std::string aName)
	{
		auto animation = std::make_shared<Animation>();
		animation->Name = std::move(aName);
		animation->Duration = static_cast<float>(aSourceAnimation.Duration);
		animation->FramesPerSecond = aSourceAnimation.FramesPerSecond;
		animation->Frames.reserve(aSourceAnimation.Frames.size());

		for (const TGA::FBX::Animation::Frame& sourceFrame : aSourceAnimation.Frames)
		{
			Animation::Frame frame;
			frame.Transforms.reserve(sourceFrame.LocalTransforms.size());
			for (const auto& [jointName, sourceTransform] : sourceFrame.LocalTransforms)
			{
				frame.Transforms.emplace(jointName, ConvertMatrix(sourceTransform));
			}
			animation->Frames.push_back(std::move(frame));
		}

		return animation;
	}

	void AppendElement(
		const TGA::FBX::Mesh::Element& aSourceElement,
		std::vector<Mesh::Element>& outElements,
		std::vector<Vertex>& outVertices,
		std::vector<unsigned>& outIndices)
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
			outVertices.push_back(ConvertVertex(sourceVertex, DefaultVertexColor));
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

void MeshLibrary::Initialize(const std::filesystem::path& aContentRoot)
{
	myContentRoot = aContentRoot;
	RegisterPrimitiveMeshes();
	LoadFBXMesh("Meshes/Props/SM_Chest.fbx");
	LoadFBXMesh("Meshes/Props/SM_Color_Checker.fbx");
	LoadFBXMesh("Meshes/Characters/TGA_Bro/SK_C_TGA_Bro.fbx");
	LoadFBXAnimation("SK_C_TGA_Bro", "Walk", "Animations/Characters/TGA_Bro/Locomotion/A_C_TGA_Bro_Walk.fbx");
	LoadFBXAnimation("SK_C_TGA_Bro", "Run", "Animations/Characters/TGA_Bro/Locomotion/A_C_TGA_Bro_Run.fbx");
	LoadFBXAnimation("SK_C_TGA_Bro", "Wave", "Animations/Characters/TGA_Bro/Idle/A_C_TGA_Bro_Idle_Wave.fbx");
	LoadFBXAnimation("SK_C_TGA_Bro", "Breathing", "Animations/Characters/TGA_Bro/Idle/A_C_TGA_Bro_Idle_Brething.fbx");
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

	for (const TGA::FBX::Mesh::Element& importedElement : importedMesh.Elements)
	{
		AppendElement(importedElement, elements, vertices, indices);
	}

	if (elements.empty())
	{
		MVLOG(Warning, "Could not load FBX mesh '{}': no renderable mesh elements were found.", resolvedPath.string());
		return false;
	}

	const std::string meshName = resolvedPath.stem().string();
	auto mesh = std::make_shared<Mesh>();
	mesh->Initialize(meshName, std::move(elements), std::move(vertices), std::move(indices));
	if (!importedMesh.Skeleton.Bones.empty())
	{
		mesh->SetSkeleton(ConvertSkeleton(importedMesh.Skeleton));
	}
	RegisterMesh(meshName, mesh);

	MVLOG(Log, "Loaded FBX mesh '{}' with {} elements.", meshName, importedMesh.Elements.size());
	return true;
}

bool MeshLibrary::LoadFBXAnimation(std::string_view aMeshName, std::string aAnimationName, const std::filesystem::path& aPath)
{
	std::shared_ptr<Mesh> mesh = GetMesh(aMeshName);
	if (mesh == nullptr)
	{
		MVLOG(Warning, "Could not load animation '{}': mesh '{}' is not registered.", aPath.string(), std::string(aMeshName));
		return false;
	}

	const std::filesystem::path resolvedPath = ResolvePath(aPath);
	if (resolvedPath.empty())
	{
		MVLOG(Warning, "Could not load animation '{}': file was not found.", aPath.string());
		return false;
	}

	TGA::FBX::Animation importedAnimation;
	if (!TGA::FBX::Importer::LoadAnimationW(resolvedPath.wstring(), importedAnimation))
	{
		MVLOG(Warning, "Could not load animation '{}': {}", resolvedPath.string(), TGA::FBX::Importer::GetLastError());
		return false;
	}

	std::shared_ptr<Animation> animation = ConvertAnimation(importedAnimation, std::move(aAnimationName));
	if (animation == nullptr || !animation->IsValid())
	{
		MVLOG(Warning, "Could not load animation '{}': imported animation was empty.", resolvedPath.string());
		return false;
	}

	mesh->AddAnimation(animation);
	MVLOG(Log, "Loaded animation '{}' for mesh '{}'.", animation->Name, std::string(aMeshName));
	return true;
}

void MeshLibrary::RegisterPrimitiveMeshes()
{
	RegisterMesh("Floor", PrimitiveMeshBuilder::CreateFloor());
	RegisterMesh("Cube", PrimitiveMeshBuilder::CreateCube());
	RegisterMesh("Pyramid", PrimitiveMeshBuilder::CreatePyramid());
	RegisterMesh("Sphere", PrimitiveMeshBuilder::CreateSphere());
	RegisterMesh("SmoothSphere", PrimitiveMeshBuilder::CreateSmoothSphere());
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
	const std::filesystem::path candidate = aPath.is_absolute() ? aPath : myContentRoot / aPath;
	std::error_code error;
	if (std::filesystem::is_regular_file(candidate, error))
	{
		return std::filesystem::canonical(candidate, error);
	}

	return {};
}
