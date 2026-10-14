#pragma once
#include <Vector3.hpp>
#include <vector>
#include <limits>
#include <utility>
#include <cstdint>
#include <type_traits>
#include <Heap.hpp>
#include <cassert>

template <typename Type>
class Pathfinder
{
	static constexpr uint32_t invalidIndex = (std::numeric_limits<uint32_t>::max)();

	public:
		struct GraphNode;

		Pathfinder();
		void Init(std::vector<GraphNode> aGraph);
		void AStar(uint32_t aStartIndex, uint32_t aTargetIndex, std::vector<uint32_t>& outPathIndices);

	private:
		struct SearchNode;
		struct HeapNode;

		std::vector<GraphNode> myGraphNodes;
		std::vector<SearchNode> mySearchNodes;
		CommonUtilities::Heap<HeapNode, true> myHeapNodes;
		uint32_t mySearchCount;

		void AllocateMemoryForNodes();
		SearchNode& Touch(uint32_t aNodeIndex);
};

template <typename Type>
struct Pathfinder<Type>::GraphNode
{
	struct Neighbor
	{
		uint32_t index;
		Type girth;
		Type cost;
	};

	CommonUtilities::Vector3<Type> position;
	std::vector<Neighbor> neighbors;
};

template <typename Type>
inline Pathfinder<Type>::Pathfinder() : mySearchCount(0) {}

template <typename Type>
inline void Pathfinder<Type>::Init(std::vector<GraphNode> aGraph)
{
	myGraphNodes = std::move(aGraph);
	AllocateMemoryForNodes();
}

template <typename Type>
inline void Pathfinder<Type>::AStar(uint32_t aStartIndex, uint32_t aTargetIndex, std::vector<uint32_t>& outPathIndices)
{
	assert(!myGraphNodes.empty() && "Pathfinder::AStar() was called before being initialized.");

	const uint32_t graphSize = static_cast<uint32_t>(myGraphNodes.size());

	assert(aStartIndex < graphSize && "Pathfinder::AStar() was called with a start index outside the graph.");
	assert(aTargetIndex < graphSize && "Pathfinder::AStar() was called with a target index outside the graph.");

	outPathIndices.clear();
	myHeapNodes.Clear();

	if (++mySearchCount == 0)
	{
		for (SearchNode& node : mySearchNodes)
		{
			node.searchStamp = 0;
		}

		mySearchCount = 1;
	}

	if (aStartIndex == aTargetIndex)
	{
		outPathIndices.push_back(aTargetIndex);
		return;
	}

	const CommonUtilities::Vector3<Type>& endPosition = myGraphNodes[aTargetIndex].position;
	Touch(aStartIndex).bestTraversedDist = Type{ 0 };

	myHeapNodes.Enqueue({
		.nodeIndex = aStartIndex,
		.targetDist = Type{ 0 },
		.totalDist = Type{ 0 }
	});

	while (0 < myHeapNodes.GetSize())
	{
		const HeapNode current = myHeapNodes.Dequeue();
		SearchNode& searchNode = mySearchNodes[current.nodeIndex];

		if (searchNode.visited)
		{
			continue;
		}

		if (current.nodeIndex == aTargetIndex)
		{
			break;
		}

		searchNode.visited = true;
		const Type traversedDistance = searchNode.bestTraversedDist;

		for (const auto& neighbor : myGraphNodes[current.nodeIndex].neighbors)
		{
			assert(neighbor.index < graphSize && "Pathfinder::AStar() found a neighbor index outside of the graph.");

			SearchNode& neighborSearchNode = Touch(neighbor.index);

			if (neighborSearchNode.visited)
			{
				continue;
			}

			const Type newTraversedDist = traversedDistance + neighbor.cost;

			if (newTraversedDist < neighborSearchNode.bestTraversedDist)
			{
				const bool isFirstDiscovery = neighborSearchNode.bestTraversedDist == (std::numeric_limits<Type>::max)();

				if (isFirstDiscovery)
				{
					const CommonUtilities::Vector3<Type>& neighborPosition = myGraphNodes[neighbor.index].position;
					neighborSearchNode.estimatedRemainingDist = (endPosition - neighborPosition).Length();
				}

				neighborSearchNode.bestTraversedDist = newTraversedDist;
				neighborSearchNode.prevIndex = current.nodeIndex;

				myHeapNodes.Enqueue({
					.nodeIndex = neighbor.index,
					.targetDist = neighborSearchNode.estimatedRemainingDist,
					.totalDist = newTraversedDist + neighborSearchNode.estimatedRemainingDist
				});
			}
		}
	}

	if (Touch(aTargetIndex).prevIndex == invalidIndex)
	{
		return;
	}

	for (uint32_t nodeIndex = aTargetIndex; nodeIndex != invalidIndex; nodeIndex = mySearchNodes[nodeIndex].prevIndex)
	{
		outPathIndices.push_back(nodeIndex);
	}
}

template <typename Type>
inline void Pathfinder<Type>::AllocateMemoryForNodes()
{
	assert(myGraphNodes.size() < invalidIndex && "Pathfinder::Init() was called with a graph too large for 32-bit indices.");
	const uint32_t graphSize = static_cast<uint32_t>(myGraphNodes.size());
	mySearchNodes.resize(graphSize);
	myHeapNodes.Reserve(graphSize);
}

template <typename Type>
inline Pathfinder<Type>::SearchNode& Pathfinder<Type>::Touch(uint32_t aNodeIndex)
{
	SearchNode& node = mySearchNodes[aNodeIndex];

	if (node.searchStamp != mySearchCount)
	{
		node.searchStamp = mySearchCount;
		node.prevIndex = invalidIndex;
		node.bestTraversedDist = (std::numeric_limits<Type>::max)();
		node.visited = false;
	}

	return node;
}

template <typename Type>
struct Pathfinder<Type>::SearchNode
{
	uint32_t searchStamp = 0;
	uint32_t prevIndex = invalidIndex;
	Type bestTraversedDist = (std::numeric_limits<Type>::max)();
	Type estimatedRemainingDist = Type{ 0 };
	bool visited = false;
};

template <typename Type>
struct Pathfinder<Type>::HeapNode
{
	uint32_t nodeIndex;
	Type targetDist;
	Type totalDist;

	bool operator < (const HeapNode& anOther) const
	{
		if constexpr (std::is_floating_point_v<Type>)
		{
			constexpr Type relativeTolerance = static_cast<Type>(1e-6);
			const Type tolerance = relativeTolerance * (std::max)(totalDist, anOther.totalDist);

			if (tolerance <= std::abs(totalDist - anOther.totalDist))
			{
				return totalDist < anOther.totalDist;
			}
		}
		else if (totalDist != anOther.totalDist)
		{
			return totalDist < anOther.totalDist;
		}

		return targetDist < anOther.targetDist;
	}
};
