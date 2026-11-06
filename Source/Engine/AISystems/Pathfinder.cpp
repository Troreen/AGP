#include "Pathfinder.hpp"
#include <Vector4.hpp>
#include <GraphicsEngine/Drawers/LineDrawer.h>
#include <cassert>
#include <utility>
#include <algorithm>
#include <limits>
#include <array>

struct Pathfinder::NavMeshQueryResult
{
	CU::Vector3f nearestPoint;
	uint32_t polyIndex = (std::numeric_limits<uint32_t>::max)();
	bool wasValid = false;
};

void Pathfinder::Init(std::vector<uint32_t>&& anIndexList, std::vector<CU::Vector3f>&& aVertexList)
{
	myIndices.clear();
	myVertices.clear();

	myIndices = std::move(anIndexList);
	myVertices = std::move(aVertexList);
	myProjectedVertices.resize(myVertices.size());

	AStar::InitializeData(myIndices, myVertices, myPersistentGraphData);
}

void Pathfinder::Update()
{
	// TODO: Store timestamp (maybe not?). Should only pathfind once per tick (unless ImmediateRequest was called).

	LineDrawer& drawer = LineDrawer::Get();
	const CU::Vector4f navMeshColor{ 0.0f, 1.0f, 1.0f, 1.0f };

	for (uint32_t i = 0; i < myIndices.size(); i += 3)
	{
		for (uint32_t j = 0; j < 3; ++j)
		{
			const CU::Vector3f vertexA = myVertices[myIndices[i + j]];
			const CU::Vector3f vertexB = myVertices[myIndices[i + ((j + 1) % 3)]];

			drawer.DrawLine(vertexA, vertexB, navMeshColor);
		}
	}

	if (testWaypoints.size() >= 2)
	{
		const CU::Vector4f pathColor{ 1.0f, 0.0f, 0.0f, 1.0f };
		const CU::Vector4f flagColor{ 0.0f, 0.0f, 1.0f, 1.0f };

		const CU::Vector3f flagVector{ 0.0f, 100.0f, 0.0f };

		for (size_t i = 1; i < testWaypoints.size(); ++i)
		{
			const CU::Vector3f& startPoint = testWaypoints[i - 1];
			const CU::Vector3f& targetPoint = testWaypoints[i];

			drawer.DrawLine(startPoint, targetPoint, pathColor);
			drawer.DrawLine(startPoint, startPoint + flagVector, flagColor);
		}
	}
}

void Pathfinder::Clear()
{
	myIndices.clear();
	myVertices.clear();
	myProjectedVertices.clear();
	myPersistentGraphData.nodeIndicesFromPolyIndex.clear();
	myPersistentGraphData.graphNodes.clear();
	myPersistentGraphData.searchNodes.clear();
	myPersistentGraphData.heapNodes.Clear();
	myPersistentGraphData.searchCount = 0;
}

void Pathfinder::ImmediateRequest(const CU::Vector3f& aStartPos, const CU::Vector3f& aTargetPos, std::vector<CU::Vector3f>& outWaypoints)
{
	RunPathfinding(GetClosestValidGeometry(aStartPos), GetClosestValidGeometry(aTargetPos), outWaypoints);
	testWaypoints = outWaypoints;
}

void Pathfinder::ImmediateRequest(const CU::Vector3f& aStartPos, const CU::Ray<float>& aTargetRay, std::vector<CU::Vector3f>& outWaypoints)
{
	RunPathfinding(GetClosestValidGeometry(aStartPos), GetClosestValidGeometry(aTargetRay), outWaypoints);
	testWaypoints = outWaypoints;
}

bool Pathfinder::GetIsInitialized() const
{
	return myIndices.empty();
}

Pathfinder::NavMeshQueryResult Pathfinder::GetClosestValidGeometry(const CU::Vector3f& aPosition) const
{
	NavMeshQueryResult result
	{
		.nearestPoint = CU::Vector3f{ 0.0f, 0.0f, 0.0f },
		.polyIndex = (std::numeric_limits<uint32_t>::max)(),
		.wasValid = false
	};

	float nearestDistSqr = (std::numeric_limits<float>::max)();

	for (size_t i = 0; i + 2 < myIndices.size(); i += 3)
	{
		const CU::Vector3f toVertices[3]
		{
			myVertices[myIndices[i]] - aPosition,
			myVertices[myIndices[i + 1]] - aPosition,
			myVertices[myIndices[i + 2]] - aPosition
		};

		const CU::Vector3f faceNormal = (toVertices[1] - toVertices[0]).Cross(toVertices[2] - toVertices[0]);
		const float faceNormalLenSqr = faceNormal.LengthSqr();

		const float shadowWeightA = faceNormal.Dot(toVertices[1].Cross(toVertices[2]));
		const float shadowWeightB = faceNormal.Dot(toVertices[2].Cross(toVertices[0]));
		const float shadowWeightC = faceNormalLenSqr - shadowWeightA - shadowWeightB;

		CU::Vector3f toCandidatePos;
		float toCandidateDistSqr = (std::numeric_limits<float>::max)();

		if (0.0f <= shadowWeightA && 0.0f <= shadowWeightB && 0.0f <= shadowWeightC && 0.0f < faceNormalLenSqr)
		{
			toCandidatePos = faceNormal * (faceNormal.Dot(toVertices[0]) / faceNormalLenSqr);
			toCandidateDistSqr = toCandidatePos.LengthSqr();
		}
		else
		{
			toCandidateDistSqr = (std::numeric_limits<float>::max)();

			for (uint8_t edgeStartIdx = 0; edgeStartIdx < 3; ++edgeStartIdx)
			{
				const uint8_t edgeEndIdx = (edgeStartIdx == 2) ? 0 : edgeStartIdx + 1;

				const CU::Vector3f edge = toVertices[edgeEndIdx] - toVertices[edgeStartIdx];
				const float edgeLenSqr = edge.LengthSqr();

				const float alongEdgeRatio = (0.0f < edgeLenSqr)
					? std::clamp(-toVertices[edgeStartIdx].Dot(edge) / edgeLenSqr, 0.0f, 1.0f)
					: 0.0f;

				const CU::Vector3f toEdgePoint = toVertices[edgeStartIdx] + edge * alongEdgeRatio;
				const float toEdgePointDistSqr = toEdgePoint.LengthSqr();

				if (toEdgePointDistSqr < toCandidateDistSqr)
				{
					toCandidateDistSqr = toEdgePointDistSqr;
					toCandidatePos = toEdgePoint;
				}
			}
		}

		if (nearestDistSqr <= toCandidateDistSqr)
		{
			continue;
		}

		nearestDistSqr = toCandidateDistSqr;

		result.nearestPoint = aPosition + toCandidatePos;
		result.polyIndex = static_cast<uint32_t>(i / 3);
	}

	return result;
}

Pathfinder::NavMeshQueryResult Pathfinder::GetClosestValidGeometry(const CU::Ray<float>& aRay)
{
	const CU::Vector3f& rayPos = aRay.GetOrigin();
	const CU::Vector3f& rayDir = aRay.GetDirection();

	const CU::Vector3f rayBasisHelperAxis = (std::abs(rayDir.y) < 0.9f)
		? CU::Vector3f{ 0.0f, 1.0f, 0.0f }
		: CU::Vector3f{ 0.0f, 0.0f, 1.0f };

	const CU::Vector3f rayBasisX = rayBasisHelperAxis.Cross(rayDir).GetNormalized();
	const CU::Vector3f rayBasisY = rayDir.Cross(rayBasisX);

	{
		const size_t vertexCount = myVertices.size();

		const CU::Vector3f* vertexData = myVertices.data();
		CU::Vector2f* projectedData = myProjectedVertices.data();

		for (size_t i = 0; i < vertexCount; ++i)
		{
			const CU::Vector3f toVertex = vertexData[i] - rayPos;
			projectedData[i] = CU::Vector2f{ toVertex.Dot(rayBasisX), toVertex.Dot(rayBasisY) };
		}
	}

	float nearestHitDist = (std::numeric_limits<float>::max)();

	NavMeshQueryResult result
	{
		.nearestPoint = CU::Vector3f{ 0.0f, 0.0f, 0.0f },
		.polyIndex = (std::numeric_limits<uint32_t>::max)(),
		.wasValid = false
	};

	for (size_t i = 0; i + 2 < myIndices.size(); i += 3)
	{
		const uint32_t vertexIndexA = myIndices[i];
		const uint32_t vertexIndexB = myIndices[i + 1];
		const uint32_t vertexIndexC = myIndices[i + 2];

		const CU::Vector2f& projectedA = myProjectedVertices[vertexIndexA];
		const CU::Vector2f& projectedB = myProjectedVertices[vertexIndexB];
		const CU::Vector2f& projectedC = myProjectedVertices[vertexIndexC];

		const float weightA = projectedB.Perp(projectedC);
		const float weightB = projectedC.Perp(projectedA);
		const float weightC = projectedA.Perp(projectedB);

		const float minWeight = (std::min)({ weightA, weightB, weightC });
		const float maxWeight = (std::max)({ weightA, weightB, weightC });

		if (minWeight < 0.0f && 0.0f < maxWeight)
		{
			continue;
		}

		const float weightSum = weightA + weightB + weightC;

		if (weightSum == 0.0f)
		{
			continue;
		}

		const CU::Vector3f toVertexA = myVertices[vertexIndexA] - rayPos;
		const CU::Vector3f toVertexB = myVertices[vertexIndexB] - rayPos;
		const CU::Vector3f toVertexC = myVertices[vertexIndexC] - rayPos;
		const CU::Vector3f toHitPos = (toVertexA * weightA + toVertexB * weightB + toVertexC * weightC) / weightSum;

		const float hitDist = toHitPos.Dot(rayDir);

		if (hitDist < 0.0f || nearestHitDist <= hitDist)
		{
			continue;
		}

		nearestHitDist = hitDist;

		result.nearestPoint = rayPos + toHitPos;
		result.polyIndex = static_cast<uint32_t>(i / 3);
		result.wasValid = true;
	}

	if (result.wasValid)
	{
		return result;
	}

	float nearestDistSqr = (std::numeric_limits<float>::max)();

	for (size_t i = 0; i + 2 < myIndices.size(); i += 3)
	{
		const uint32_t vertexIndexA = myIndices[i];
		const uint32_t vertexIndexB = myIndices[i + 1];
		const uint32_t vertexIndexC = myIndices[i + 2];

		const CU::Vector2f& projectedA = myProjectedVertices[vertexIndexA];
		const CU::Vector2f& projectedB = myProjectedVertices[vertexIndexB];
		const CU::Vector2f& projectedC = myProjectedVertices[vertexIndexC];

		const float nearestBoundX = (std::max)({
			0.0f,
			(std::min)({ projectedA.x, projectedB.x, projectedC.x }),
			-(std::max)({ projectedA.x, projectedB.x, projectedC.x })
		});

		const float nearestBoundY = (std::max)({
			0.0f,
			(std::min)({ projectedA.y, projectedB.y, projectedC.y }),
			-(std::max)({ projectedA.y, projectedB.y, projectedC.y })
		});

		const float boundDistSqr = nearestBoundX * nearestBoundX + nearestBoundY * nearestBoundY;

		if (nearestDistSqr <= boundDistSqr)
		{
			continue;
		}

		const float weightA = projectedB.Perp(projectedC);
		const float weightB = projectedC.Perp(projectedA);
		const float weightC = projectedA.Perp(projectedB);

		const float minWeight = (std::min)({ weightA, weightB, weightC });
		const float maxWeight = (std::max)({ weightA, weightB, weightC });
		const float weightSum = weightA + weightB + weightC;

		const bool isIntersectingBehindCamera = !(minWeight < 0.0f && 0.0f < maxWeight) && weightSum != 0.0f;

		const CU::Vector3f toVertices[3]
		{
			myVertices[vertexIndexA] - rayPos,
			myVertices[vertexIndexB] - rayPos,
			myVertices[vertexIndexC] - rayPos
		};

		CU::Vector3f toCandidatePos;
		float toCandidateDistSqr = (std::numeric_limits<float>::max)();
		bool measureFromCamera = isIntersectingBehindCamera;

		if (!measureFromCamera)
		{
			const CU::Vector2f* projectedVertices[3]{ &projectedA, &projectedB, &projectedC };

			for (uint8_t edgeStartIdx = 0; edgeStartIdx < 3; ++edgeStartIdx)
			{
				const uint8_t edgeEndIdx = (edgeStartIdx == 2) ? 0 : edgeStartIdx + 1;

				const CU::Vector2f projectedEdge = *projectedVertices[edgeEndIdx] - *projectedVertices[edgeStartIdx];
				const float projectedEdgeLenSqr = projectedEdge.LengthSqr();

				const float alongEdgeRatio = (0.0f < projectedEdgeLenSqr)
					? std::clamp(-projectedVertices[edgeStartIdx]->Dot(projectedEdge) / projectedEdgeLenSqr, 0.0f, 1.0f)
					: 0.0f;

				const float toEdgePointDistSqr = (*projectedVertices[edgeStartIdx] + projectedEdge * alongEdgeRatio).LengthSqr();

				if (toEdgePointDistSqr < toCandidateDistSqr)
				{
					toCandidateDistSqr = toEdgePointDistSqr;
					toCandidatePos = toVertices[edgeStartIdx] + (toVertices[edgeEndIdx] - toVertices[edgeStartIdx]) * alongEdgeRatio;
				}
			}

			measureFromCamera = toCandidatePos.Dot(rayDir) < 0.0f;
		}

		if (measureFromCamera)
		{
			const CU::Vector3f faceNormal = (toVertices[1] - toVertices[0]).Cross(toVertices[2] - toVertices[0]);
			const float faceNormalLenSqr = faceNormal.LengthSqr();

			const float shadowWeightA = faceNormal.Dot(toVertices[1].Cross(toVertices[2]));
			const float shadowWeightB = faceNormal.Dot(toVertices[2].Cross(toVertices[0]));
			const float shadowWeightC = faceNormalLenSqr - shadowWeightA - shadowWeightB;

			if (0.0f <= shadowWeightA && 0.0f <= shadowWeightB && 0.0f <= shadowWeightC && 0.0f < faceNormalLenSqr)
			{
				toCandidatePos = faceNormal * (faceNormal.Dot(toVertices[0]) / faceNormalLenSqr);
				toCandidateDistSqr = toCandidatePos.LengthSqr();
			}
			else
			{
				toCandidateDistSqr = (std::numeric_limits<float>::max)();

				for (uint8_t edgeStartIdx = 0; edgeStartIdx < 3; ++edgeStartIdx)
				{
					const uint8_t edgeEndIdx = (edgeStartIdx == 2) ? 0 : edgeStartIdx + 1;

					const CU::Vector3f edge = toVertices[edgeEndIdx] - toVertices[edgeStartIdx];
					const float edgeLenSqr = edge.LengthSqr();

					const float alongEdgeRatio = (0.0f < edgeLenSqr)
						? std::clamp(-toVertices[edgeStartIdx].Dot(edge) / edgeLenSqr, 0.0f, 1.0f)
						: 0.0f;

					const CU::Vector3f toEdgePoint = toVertices[edgeStartIdx] + edge * alongEdgeRatio;
					const float toEdgePointDistSqr = toEdgePoint.LengthSqr();

					if (toEdgePointDistSqr < toCandidateDistSqr)
					{
						toCandidateDistSqr = toEdgePointDistSqr;
						toCandidatePos = toEdgePoint;
					}
				}
			}
		}

		if (nearestDistSqr <= toCandidateDistSqr)
		{
			continue;
		}

		nearestDistSqr = toCandidateDistSqr;

		result.nearestPoint = rayPos + toCandidatePos;
		result.polyIndex = static_cast<uint32_t>(i / 3);
	}
	
	return result;
}

bool Pathfinder::RunPathfinding(const NavMeshQueryResult& startQueryResult, const NavMeshQueryResult& targetQueryResult, std::vector<CU::Vector3f>& outWaypoints)
{
	std::vector<uint32_t> pathNodeIndices;
	bool wasSearchValid = AStar::Search(myPersistentGraphData, startQueryResult.polyIndex, targetQueryResult.polyIndex, startQueryResult.nearestPoint, targetQueryResult.nearestPoint, pathNodeIndices);

	if (!wasSearchValid)
	{
		return false;
	}

	AStar::Funnel(myPersistentGraphData, startQueryResult.nearestPoint, targetQueryResult.nearestPoint, pathNodeIndices, outWaypoints);
	return true;
}
