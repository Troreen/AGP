#pragma once

#include "Vector2.hpp"

namespace CommonUtilities
{
	struct Rect2D
	{
		Vector2<float> myMin;
		Vector2<float> myMax;

		static Rect2D FromCenterSize(const Vector2<float>& aCenter, const Vector2<float>& aSize)
		{
			const Vector2<float> halfSize = aSize * 0.5f;
			return { aCenter - halfSize, aCenter + halfSize };
		}

		static Rect2D FromMinSize(const Vector2<float>& aMin, const Vector2<float>& aSize)
		{
			return { aMin, aMin + aSize };
		}

		Vector2<float> GetCenter() const
		{
			return (myMin + myMax) * 0.5f;
		}

		Vector2<float> GetSize() const
		{
			return myMax - myMin;
		}

		bool Contains(const Vector2<float>& aPoint) const
		{
			return aPoint.x >= myMin.x && aPoint.x <= myMax.x
				&& aPoint.y >= myMin.y && aPoint.y <= myMax.y;
		}

		bool Contains(const Rect2D& aRect) const
		{
			return aRect.myMin.x >= myMin.x && aRect.myMax.x <= myMax.x
				&& aRect.myMin.y >= myMin.y && aRect.myMax.y <= myMax.y;
		}

		bool Intersects(const Rect2D& aRect) const
		{
			return myMin.x <= aRect.myMax.x && myMax.x >= aRect.myMin.x
				&& myMin.y <= aRect.myMax.y && myMax.y >= aRect.myMin.y;
		}
	};
}
