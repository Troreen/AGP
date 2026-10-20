#include "Pathfinder.hpp"
#include <cassert>
#include <utility>

struct Pathfinder::NavMeshQueryResult
{
	Vector3f nearestPosition;
	uint32_t polyNodeIndex; // TODO: our nodes aren't polys, but edges.
	bool wasValidPoint;
};

void Pathfinder::Init(std::vector<uint32_t>&& anIndexList, std::vector<Vector3f>&& aVertexList)
{
	myIndices.clear();
	myVertices.clear();

	myIndices = std::move(anIndexList);
	myVertices = std::move(aVertexList);

	AStar::InitializeData(myIndices, myVertices, myGraphData);
}

void Pathfinder::Update()
{
	// TODO: Store timestamp. Should only pathfind once per tick (unless ImmediateRequest was called).


}

void Pathfinder::Clear()
{
	myIndices.clear();
	myVertices.clear();
	myGraphData.graphNodes.clear();
	myGraphData.searchNodes.clear();
	myGraphData.heapNodes.Clear();
	myGraphData.searchCount = 0;
}

void Pathfinder::ImmediateRequest(const Vector3f& aStartPos, const Vector3f& aTargetPos, std::vector<Vector3f>& outWaypoints) const
{
	// TODO: Immediately perform a search and calculate waypoints.
	(void)aStartPos;
	(void)aTargetPos;
	(void)outWaypoints;
}

void Pathfinder::ImmediateRequest(const Vector3f& aStartPos, const Ray<float>& aTargetRay, std::vector<Vector3f>& outWaypoints) const
{
	// TODO: Immediately perform a search and calculate waypoints.
	(void)aStartPos;
	(void)aTargetRay;
	(void)outWaypoints;
}

Pathfinder::NavMeshQueryResult Pathfinder::GetClosestValidGeometry(const Vector3f& aPosition) const
{
	// TODO
	(void)aPosition;
	return NavMeshQueryResult();
}

Pathfinder::NavMeshQueryResult Pathfinder::GetClosestValidGeometry(const Ray<float>& aRay) const
{
	for (size_t i = 0; i < myIndices.size(); i += 3)
	{
		Vector3f vertices[3];

		for (uint8_t j = 0; j < 3; ++j)
		{
			vertices[j] = myVertices[i + j];
		}

		const Vector3f rayDir = aRay.GetDirection();
		const Vector3f toVertexVec = vertices[0] - aRay.GetOrigin();

		const Vector3f projectedVertexA = toVertexVec - rayDir * rayDir.Dot(toVertexVec);


		const Vector3f edgeVec = vertices[1] - vertices[0];


	}

	// TODO
	(void)aRay;
	return NavMeshQueryResult();
}
