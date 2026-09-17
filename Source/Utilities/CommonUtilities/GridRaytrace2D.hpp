#pragma once

#include "Grid2D.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace CommonUtilities
{
	template <typename Visitor>
	void RaytraceGridCells(const Grid2D& aGrid, const Grid2D::Vector2f& aFrom, const Grid2D::Vector2f& aTo, Visitor&& aVisitor)
	{
		if (aGrid.GetCellSize() <= 0.0f || aGrid.GetWidth() <= 0 || aGrid.GetHeight() <= 0)
		{
			return;
		}

		const Grid2D::Vector2f origin = aGrid.GetOrigin();
		const float cellSize = aGrid.GetCellSize();
		const float x0 = (aFrom.x - origin.x) / cellSize;
		const float y0 = (aFrom.y - origin.y) / cellSize;
		const float x1 = (aTo.x - origin.x) / cellSize;
		const float y1 = (aTo.y - origin.y) / cellSize;

		const float dx = std::abs(x1 - x0);
		const float dy = std::abs(y1 - y0);
		int x = std::clamp(static_cast<int>(std::floor(x0)), 0, aGrid.GetWidth() - 1);
		int y = std::clamp(static_cast<int>(std::floor(y0)), 0, aGrid.GetHeight() - 1);
		const int endX = std::clamp(static_cast<int>(std::floor(x1)), 0, aGrid.GetWidth() - 1);
		const int endY = std::clamp(static_cast<int>(std::floor(y1)), 0, aGrid.GetHeight() - 1);

		const float dtDx = dx == 0.0f ? std::numeric_limits<float>::infinity() : 1.0f / dx;
		const float dtDy = dy == 0.0f ? std::numeric_limits<float>::infinity() : 1.0f / dy;
		int cellsToVisit = 1;
		int xIncrement = 0;
		int yIncrement = 0;
		float nextVertical = dtDx;
		float nextHorizontal = dtDy;

		if (dx > 0.0f)
		{
			if (x1 > x0)
			{
				xIncrement = 1;
				cellsToVisit += endX - x;
				nextVertical = (std::floor(x0) + 1.0f - x0) * dtDx;
			}
			else
			{
				xIncrement = -1;
				cellsToVisit += x - endX;
				nextVertical = (x0 - std::floor(x0)) * dtDx;
			}
		}

		if (dy > 0.0f)
		{
			if (y1 > y0)
			{
				yIncrement = 1;
				cellsToVisit += endY - y;
				nextHorizontal = (std::floor(y0) + 1.0f - y0) * dtDy;
			}
			else
			{
				yIncrement = -1;
				cellsToVisit += y - endY;
				nextHorizontal = (y0 - std::floor(y0)) * dtDy;
			}
		}

		for (int i = 0; i < cellsToVisit; ++i)
		{
			if (aGrid.IsValidCell(x, y))
			{
				const int cellIndex = aGrid.GetCellIndex(x, y);
				if (!aVisitor(cellIndex))
				{
					return;
				}
			}

			if (nextVertical < nextHorizontal)
			{
				x += xIncrement;
				nextVertical += dtDx;
			}
			else
			{
				y += yIncrement;
				nextHorizontal += dtDy;
			}
		}
	}
}
