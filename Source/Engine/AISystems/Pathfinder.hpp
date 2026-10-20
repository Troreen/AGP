#pragma once
#include "Pathfinding/AStar.h"
#include <Vector3.hpp>
#include <Ray.hpp>
#include <Heap.hpp>
#include <unordered_map>
#include <vector>
#include <limits>
#include <utility>
#include <cstdint>
#include <functional>

using namespace CommonUtilities;

class Pathfinder
{
	public:
		using QueueRequestHandler = std::function<void(std::vector<Vector3f>& outWaypoints)>;

		void Init(std::vector<uint32_t>&& anIndexList, std::vector<Vector3f>&& aVertexList);
		void Update();
		void Clear();

		template <typename T>
		void QueueRequest(const T* aRequester, const Vector3f& aStartPos, const Vector3f& aTargetPos, std::vector<Vector3f>& outWaypoints);

		template <typename T>
		void QueueRequest(const T* aRequester, const Vector3f& aStartPos, const Ray<float>& aTargetRay, std::vector<Vector3f>& outWaypoints);

		void ImmediateRequest(const Vector3f& aStartPos, const Vector3f& aTargetPos, std::vector<Vector3f>& outWaypoints) const;
		void ImmediateRequest(const Vector3f& aStartPos, const Ray<float>& aTargetRay, std::vector<Vector3f>& outWaypoints) const;
		
	private:
		struct NavMeshQueryResult;

		std::vector<uint32_t> myIndices;
		std::vector<Vector3f> myVertices;
		AStar::PersistentData myGraphData;

		NavMeshQueryResult GetClosestValidGeometry(const Vector3f& aPosition) const;
		NavMeshQueryResult GetClosestValidGeometry(const Ray<float>& aRay) const;
};

template <typename T>
inline void Pathfinder::QueueRequest(const T* aRequester, const Vector3f& aStartPos, const Vector3f& aTargetPos, std::vector<Vector3f>& outWaypoints)
{
	// TODO
	(void)aRequester;
	(void)aStartPos;
	(void)aTargetPos;
	(void)outWaypoints;
}

template <typename T>
inline void Pathfinder::QueueRequest(const T* aRequester, const Vector3f& aStartPos, const Ray<float>& aTargetRay, std::vector<Vector3f>& outWaypoints)
{
	// TODO
	(void)aRequester;
	(void)aStartPos;
	(void)aTargetRay;
	(void)outWaypoints;
}
