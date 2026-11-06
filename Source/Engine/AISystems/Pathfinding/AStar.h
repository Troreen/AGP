#pragma once
#include <Vector3.hpp>
#include <Heap.hpp>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_map>

namespace AStar
{
	constexpr uint32_t invalidIndex = (std::numeric_limits<uint32_t>::max)();

	struct GraphNode
	{
		struct Neighbor
		{
			uint32_t index;
			float girth;
			float cost;
		};

		CommonUtilities::Vector3f position;
		CommonUtilities::Vector3f halfPortalVec;
		std::vector<Neighbor> neighbors;
	};

	struct SearchNode
	{
		uint32_t searchStamp = 0;
		uint32_t prevIndex = invalidIndex;
		float bestTraversedDist = (std::numeric_limits<float>::max)();
		float estimatedRemainingDist = 0.0f;
		bool visited = false;
	};

	struct HeapNode
	{
		uint32_t nodeIndex;
		float targetDist;
		float totalDist;

		bool operator < (const HeapNode& anOther) const
		{
			constexpr float relativeTolerance = 1e-6f;
			const float tolerance = relativeTolerance * (std::max)(totalDist, anOther.totalDist);

			if (tolerance <= std::abs(totalDist - anOther.totalDist))
			{
				return totalDist < anOther.totalDist;
			}

			return targetDist < anOther.targetDist;
		}
	};

	struct PersistentData
	{
		std::vector<std::array<uint32_t, 3>> nodeIndicesFromPolyIndex;
		std::vector<GraphNode> graphNodes;
		std::vector<SearchNode> searchNodes;
		CommonUtilities::Heap<HeapNode, true> heapNodes;

		uint32_t searchCount;
	};

	void InitializeData(std::vector<uint32_t>& anIndexList, std::vector<CommonUtilities::Vector3f>& aVertexList, PersistentData& outGraphData);
	bool Search(PersistentData& someGraphData, uint32_t aStartPolyIndex, uint32_t aTargetPolyIndex, const CommonUtilities::Vector3f& aStartPos, const CommonUtilities::Vector3f& aTargetPos, std::vector<uint32_t>& outNodeIndices);
	void Funnel(PersistentData& someGraphData, const CommonUtilities::Vector3f& aStartPos, const CommonUtilities::Vector3f& aTargetPos, const std::vector<uint32_t>& somePathNodeIndices, std::vector<CommonUtilities::Vector3f>& outWaypoints);
	//void SmoothWaypoints(float aRadius, uint8_t anIterationCount, const std::vector<CommonUtilities::Vector3f>& someInputWaypoints, std::vector<CommonUtilities::Vector3f>& outWaypoints);

	namespace detail
	{
		SearchNode& TouchSearchNode(PersistentData& someGraphData, uint32_t aSearchNodeIndex);
	}
}
