#pragma once
#include "Pathfinding/AStar.h"
#include <CommonUtilities.h>
#include <Ray.hpp>
#include <Heap.hpp>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <functional>

class Pathfinder
{
	public:
		using QueueRequestHandler = std::function<void(std::vector<CU::Vector3f>& outWaypoints)>;

		void Init(std::vector<uint32_t>&& anIndexList, std::vector<CU::Vector3f>&& aVertexList);
		void Update();
		void Clear();

		template <typename T>
		void QueueRequest(const T* aRequester, const CU::Vector3f& aStartPos, const CU::Vector3f& aTargetPos, std::vector<CU::Vector3f>& outWaypoints);

		template <typename T>
		void QueueRequest(const T* aRequester, const CU::Vector3f& aStartPos, const CU::Ray<float>& aTargetRay, std::vector<CU::Vector3f>& outWaypoints);

		void ImmediateRequest(const CU::Vector3f& aStartPos, const CU::Vector3f& aTargetPos, std::vector<CU::Vector3f>& outWaypoints);
		void ImmediateRequest(const CU::Vector3f& aStartPos, const CU::Ray<float>& aTargetRay, std::vector<CU::Vector3f>& outWaypoints);

		bool GetIsInitialized() const;

	private:
		struct NavMeshQueryResult;

		std::vector<uint32_t> myIndices;
		std::vector<CU::Vector3f> myVertices;
		std::vector<CU::Vector2f> myProjectedVertices;

		AStar::PersistentData myPersistentGraphData;

		NavMeshQueryResult GetClosestValidGeometry(const CU::Vector3f& aPosition) const;
		NavMeshQueryResult GetClosestValidGeometry(const CU::Ray<float>& aRay);
		bool RunPathfinding(const NavMeshQueryResult& startQueryResult, const NavMeshQueryResult& targetQueryResult, std::vector<CU::Vector3f>& outWaypoints);

		// TODO: TEMPORARY TESTING; REMOVE
		std::vector<CU::Vector3f> testWaypoints;
};

template <typename T>
inline void Pathfinder::QueueRequest(const T* aRequester, const CU::Vector3f& aStartPos, const CU::Vector3f& aTargetPos, std::vector<CU::Vector3f>& outWaypoints)
{
	// TODO
	(void)aRequester;
	(void)aStartPos;
	(void)aTargetPos;
	(void)outWaypoints;
}

template <typename T>
inline void Pathfinder::QueueRequest(const T* aRequester, const CU::Vector3f& aStartPos, const CU::Ray<float>& aTargetRay, std::vector<CU::Vector3f>& outWaypoints)
{
	// TODO
	(void)aRequester;
	(void)aStartPos;
	(void)aTargetRay;
	(void)outWaypoints;
}
