#include <fbxsdk.h>
#include <cmath>
#include <iostream>

void Inspect(FbxNode* node)
{
	if (const FbxMesh* mesh = node->GetMesh())
	{
		if (mesh->GetElementNormalCount() > 0)
		{
			const auto* normals = mesh->GetElementNormal(0);
			FbxVector4 mean(0, 0, 0, 0);
			double meanLength = 0;
			for (int i = 0; i < normals->GetDirectArray().GetCount(); ++i)
			{
				const FbxVector4 normal = normals->GetDirectArray().GetAt(i);
				mean += normal;
				meanLength += normal.Length();
			}
			const int count = normals->GetDirectArray().GetCount();
			const FbxAMatrix transform = node->EvaluateGlobalTransform();
			const FbxVector4 translation = transform.GetT();
			const FbxVector4 scale = transform.GetS();
			const FbxVector4 first = normals->GetDirectArray().GetAt(0);
			const FbxVector4 asPoint = transform.MultT(first);
			const FbxVector4 asDirection = transform.MultR(first);
			std::cout << node->GetName() << ": normals=" << count << ", mean length=" << meanLength / count
				<< ", mean direction=" << mean[0] / count << ',' << mean[1] / count << ',' << mean[2] / count << '\n';
			std::cout << "Node translation=" << translation[0] << ',' << translation[1] << ',' << translation[2]
				<< ", scale=" << scale[0] << ',' << scale[1] << ',' << scale[2] << '\n';
			std::cout << "First normal=" << first[0] << ',' << first[1] << ',' << first[2]
				<< "; transformed as point=" << asPoint[0] << ',' << asPoint[1] << ',' << asPoint[2]
				<< "; transformed as direction=" << asDirection[0] << ',' << asDirection[1] << ',' << asDirection[2] << '\n';
		}
	}
	for (int i = 0; i < node->GetChildCount(); ++i)
	{
		Inspect(node->GetChild(i));
	}
}

int main(int argc, char** argv)
{
	if (argc != 2 && argc != 3)
	{
		return 2;
	}
	FbxManager* manager = FbxManager::Create();
	manager->SetIOSettings(FbxIOSettings::Create(manager, IOSROOT));
	FbxImporter* importer = FbxImporter::Create(manager, "Source probe");
	FbxScene* scene = FbxScene::Create(manager, "Source");
	if (!importer->Initialize(argv[1], -1, manager->GetIOSettings()) || !importer->Import(scene))
	{
		std::cerr << importer->GetStatus().GetErrorString() << '\n';
		manager->Destroy();
		return 1;
	}
	Inspect(scene->GetRootNode());
	if (argc == 3)
	{
		FbxNode* meshNode = nullptr;
		for (int i = 0; i < scene->GetRootNode()->GetChildCount(); ++i)
		{
			auto* child = scene->GetRootNode()->GetChild(i);
			if (child->GetMesh())
			{
				if (meshNode)
				{
					std::cerr << "Candidate bake is restricted to one root mesh\n";
					return 1;
				}
				meshNode = child;
			}
		}
		if (!meshNode)
		{
			return 1;
		}
		const FbxAMatrix transform = meshNode->EvaluateGlobalTransform();
		const FbxVector4 scale = transform.GetS();
		if (scale[0] <= 0 || std::abs(scale[0] - scale[1]) > 1e-6 || std::abs(scale[0] - scale[2]) > 1e-6 ||
			meshNode->GetGeometricTranslation(FbxNode::eSourcePivot).Length() > 1e-6 ||
			meshNode->GetGeometricRotation(FbxNode::eSourcePivot).Length() > 1e-6)
		{
			std::cerr << "Candidate bake requires uniform positive scale and no geometric offset\n";
			return 1;
		}
		FbxMesh* mesh = meshNode->GetMesh();
		for (int i = 0; i < mesh->GetControlPointsCount(); ++i)
		{
			mesh->GetControlPoints()[i] = transform.MultT(mesh->GetControlPointAt(i));
		}
		auto transformDirections = [&transform](auto* element)
		{
			if (!element)
			{
				return;
			}
			for (int i = 0; i < element->GetDirectArray().GetCount(); ++i)
			{
				FbxVector4 direction = transform.MultR(element->GetDirectArray().GetAt(i));
				direction.Normalize();
				element->GetDirectArray().SetAt(i, direction);
			}
		};
		for (int i = 0; i < mesh->GetElementNormalCount(); ++i)
		{
			transformDirections(mesh->GetElementNormal(i));
		}
		for (int i = 0; i < mesh->GetElementTangentCount(); ++i)
		{
			transformDirections(mesh->GetElementTangent(i));
		}
		for (int i = 0; i < mesh->GetElementBinormalCount(); ++i)
		{
			transformDirections(mesh->GetElementBinormal(i));
		}
		meshNode->LclTranslation.Set(FbxDouble3(0, 0, 0));
		meshNode->LclRotation.Set(FbxDouble3(0, 0, 0));
		meshNode->LclScaling.Set(FbxDouble3(1, 1, 1));
		FbxExporter* exporter = FbxExporter::Create(manager, "Candidate");
		if (!exporter->Initialize(argv[2], -1, manager->GetIOSettings()) || !exporter->Export(scene))
		{
			std::cerr << exporter->GetStatus().GetErrorString() << '\n';
			manager->Destroy();
			return 1;
		}
		std::cout << "Wrote separate transform-baked candidate: " << argv[2] << '\n';
	}
	manager->Destroy();
	return 0;
}
