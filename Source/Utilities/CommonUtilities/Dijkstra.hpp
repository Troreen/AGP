#pragma once

#include "PathfindingTypes.hpp"

#include <algorithm>
#include <array>
#include <queue>
#include <vector>

namespace CommonUtilities
{
	inline std::vector<int> Dijkstra(
		const std::vector<Tile>& aMap,
		int aStartIndex,
		int anEndIndex
	)
	{
		if (aMap.size() != TileCount)
		{
			return {};
		}

		if (aStartIndex < 0 || aStartIndex >= TileCount)
		{
			return {};
		}

		if (anEndIndex < 0 || anEndIndex >= TileCount)
		{
			return {};
		}

		if (aMap[aStartIndex] == Tile::Impassable || aMap[anEndIndex] == Tile::Impassable)
		{
			return {};
		}

		if (aStartIndex == anEndIndex)
		{
			return { aStartIndex };
		}

		struct QueueNode
		{
			int myDistance = 0;
			int myOrder = 0;
			int myIndex = 0;
		};

		struct QueueCompare
		{
			bool operator()(const QueueNode& aLeft, const QueueNode& aRight) const
			{
				if (aLeft.myDistance == aRight.myDistance)
				{
					return aLeft.myOrder > aRight.myOrder;
				}

				return aLeft.myDistance > aRight.myDistance;
			}
		};

		static const int Unvisited = -1;
		std::array<int, TileCount> distances;
		std::array<int, TileCount> previous;
		distances.fill(Unvisited);
		previous.fill(Unvisited);

		std::priority_queue<QueueNode, std::vector<QueueNode>, QueueCompare> openTiles;
		int nextQueueOrder = 0;

		distances[aStartIndex] = 0;
		openTiles.push({ 0, nextQueueOrder++, aStartIndex });

		while (!openTiles.empty())
		{
			const QueueNode current = openTiles.top();
			openTiles.pop();

			if (current.myDistance != distances[current.myIndex])
			{
				continue;
			}

			if (current.myIndex == anEndIndex)
			{
				break;
			}

			const int currentX = current.myIndex % MapWidth;
			const int currentY = current.myIndex / MapWidth;

			const auto tryVisit = [&](int aX, int aY)
			{
				if (aX < 0 || aX >= MapWidth || aY < 0 || aY >= MapHeight)
				{
					return;
				}

				const int neighborIndex = aY * MapWidth + aX;
				if (aMap[neighborIndex] == Tile::Impassable)
				{
					return;
				}

				const int newDistance = current.myDistance + 1;
				if (distances[neighborIndex] != Unvisited && distances[neighborIndex] <= newDistance)
				{
					return;
				}

				distances[neighborIndex] = newDistance;
				previous[neighborIndex] = current.myIndex;
				openTiles.push({ newDistance, nextQueueOrder++, neighborIndex });
			};

			tryVisit(currentX, currentY - 1);
			tryVisit(currentX, currentY + 1);
			tryVisit(currentX - 1, currentY);
			tryVisit(currentX + 1, currentY);
		}

		if (previous[anEndIndex] == Unvisited)
		{
			return {};
		}

		std::vector<int> path;
		for (int tileIndex = anEndIndex; tileIndex != Unvisited; tileIndex = previous[tileIndex])
		{
			path.push_back(tileIndex);
		}

		std::reverse(path.begin(), path.end());
		return path;
	}
}
