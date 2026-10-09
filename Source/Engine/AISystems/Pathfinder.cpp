#include "Pathfinder.h"
#include <Vector2.hpp>
#include <TGAFbx.h>
#include <GameFramework/ServiceLocator.h>
#include <GameFramework/AssetHandling/MeshAsset.h>
#include <GameFramework/AssetHandling/AssetRegistry.h>
#include <GraphicsEngine/Objects/Vertex.h>
#include <iostream>


Pathfinder::Pathfinder()
{
	// Work in progress
	
	//const std::shared_ptr<MeshAsset> navMeshAsset = ServiceLocator::GetInstance().GetAssetRegistry().GetAsset<MeshAsset>("Lvl_Blockout_NavMesh.fbx");
	//const std::shared_ptr<Mesh>& navMesh = navMeshAsset->GetMesh();

	//std::vector<Vertex>& vertices = navMesh->myVertices;
	//std::vector<unsigned>& indices = navMesh->myIndices;

	//for (const Vertex& vertex : vertices)
	//{
	//	std::cout << vertex.Position.x << " " << vertex.Position.y << " " << vertex.Position.z << " " << std::endl;
	//}
}

Pathfinder::~Pathfinder()
{
}

void Pathfinder::QueueRequest(const Vector3f& aStartPos, const Vector3f& aTargetPos, std::function<void(std::vector<Vector3f>& outWaypoints)> aResultHandler)
{
	(void)aStartPos;
	(void)aTargetPos;
	(void)aResultHandler;
}

void Pathfinder::ImmediateRequest(const Vector3<float>& aStartPos, const Vector3<float>& aTargetPos, std::vector<Vector3<float>>& outWaypoints) const
{
	(void)aStartPos;
	(void)aTargetPos;
	(void)outWaypoints;
}

void Pathfinder::BuildNavMesh()
{
}

