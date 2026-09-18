#pragma once

#include "Vector2.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace CommonUtilities
{
	class Grid2D
	{
	public:
		using Vector2f = Vector2<float>;

		Grid2D() = default;

		Grid2D(const Vector2f& anOrigin, float aCellSize, int aWidth, int aHeight)
		{
			Init(anOrigin, aCellSize, aWidth, aHeight);
		}

		void Init(const Vector2f& anOrigin, float aCellSize, int aWidth, int aHeight)
		{
			myOrigin = anOrigin;
			myCellSize = aCellSize;
			myWidth = aWidth;
			myHeight = aHeight;
			myCells.clear();
			myCells.resize(static_cast<size_t>(myWidth * myHeight));
		}

		void Clear()
		{
			for (std::vector<int>& cell : myCells)
			{
				cell.clear();
			}
		}

		void InsertCircle(int anObjectId, const Vector2f& aCenter, float aRadius)
		{
			VisitCellsOverlappingCircle(aCenter, aRadius, [this, anObjectId](int aCellIndex)
			{
				myCells[static_cast<size_t>(aCellIndex)].push_back(anObjectId);
				return true;
			});
		}

		int GetCellIndex(const Vector2f& aPosition) const
		{
			const int x = static_cast<int>(std::floor((aPosition.x - myOrigin.x) / myCellSize));
			const int y = static_cast<int>(std::floor((aPosition.y - myOrigin.y) / myCellSize));
			return GetCellIndex(x, y);
		}

		int GetCellIndex(int aX, int aY) const
		{
			if (!IsValidCell(aX, aY))
			{
				return -1;
			}

			return aY * myWidth + aX;
		}

		bool GetCellCoordinates(int aCellIndex, int& anOutX, int& anOutY) const
		{
			if (aCellIndex < 0 || aCellIndex >= GetCellCount())
			{
				return false;
			}

			anOutX = aCellIndex % myWidth;
			anOutY = aCellIndex / myWidth;
			return true;
		}

		const std::vector<int>& GetObjectsInCell(int aCellIndex) const
		{
			static const std::vector<int> emptyCell;
			if (aCellIndex < 0 || aCellIndex >= GetCellCount())
			{
				return emptyCell;
			}

			return myCells[static_cast<size_t>(aCellIndex)];
		}

		template <typename Visitor>
		void VisitCellsOverlappingCircle(const Vector2f& aCenter, float aRadius, Visitor&& aVisitor) const
		{
			if (myCellSize <= 0.0f || myWidth <= 0 || myHeight <= 0)
			{
				return;
			}

			int minX = static_cast<int>(std::floor((aCenter.x - aRadius - myOrigin.x) / myCellSize));
			int maxX = static_cast<int>(std::floor((aCenter.x + aRadius - myOrigin.x) / myCellSize));
			int minY = static_cast<int>(std::floor((aCenter.y - aRadius - myOrigin.y) / myCellSize));
			int maxY = static_cast<int>(std::floor((aCenter.y + aRadius - myOrigin.y) / myCellSize));

			minX = std::clamp(minX, 0, myWidth - 1);
			maxX = std::clamp(maxX, 0, myWidth - 1);
			minY = std::clamp(minY, 0, myHeight - 1);
			maxY = std::clamp(maxY, 0, myHeight - 1);

			for (int y = minY; y <= maxY; ++y)
			{
				for (int x = minX; x <= maxX; ++x)
				{
					if (!DoesCellOverlapCircle(x, y, aCenter, aRadius))
					{
						continue;
					}

					if (!aVisitor(GetCellIndex(x, y)))
					{
						return;
					}
				}
			}
		}

		bool IsValidCell(int aX, int aY) const
		{
			return aX >= 0 && aX < myWidth && aY >= 0 && aY < myHeight;
		}

		const Vector2f& GetOrigin() const { return myOrigin; }
		float GetCellSize() const { return myCellSize; }
		int GetWidth() const { return myWidth; }
		int GetHeight() const { return myHeight; }
		int GetCellCount() const { return myWidth * myHeight; }

	private:
		bool DoesCellOverlapCircle(int aX, int aY, const Vector2f& aCenter, float aRadius) const
		{
			const float minX = myOrigin.x + static_cast<float>(aX) * myCellSize;
			const float minY = myOrigin.y + static_cast<float>(aY) * myCellSize;
			const float maxX = minX + myCellSize;
			const float maxY = minY + myCellSize;

			const float closestX = std::clamp(aCenter.x, minX, maxX);
			const float closestY = std::clamp(aCenter.y, minY, maxY);
			const float deltaX = aCenter.x - closestX;
			const float deltaY = aCenter.y - closestY;

			return deltaX * deltaX + deltaY * deltaY <= aRadius * aRadius;
		}

		Vector2f myOrigin = Vector2f::Zero;
		float myCellSize = 1.0f;
		int myWidth = 0;
		int myHeight = 0;
		std::vector<std::vector<int>> myCells;
	};
}
