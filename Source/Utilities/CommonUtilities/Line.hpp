#pragma once
#include "Vector2.hpp"
#include <cmath>

namespace CommonUtilities
{
	template <typename T>
	class Line
	{
	public:
		// Default constructor: there is no line, the normal is the zero vector.
		Line();

		// Constructor that takes two points that define the line, the direction is aPoint1 - aPoint0.
		Line(const Vector2<T>& aPoint0, const Vector2<T>& aPoint1);

		// Init the line with two points, the same as the constructor above.
		void InitWith2Points(const Vector2<T>& aPoint0, const Vector2<T>& aPoint1);

		// Init the line with a point and a direction.
		void InitWithPointAndDirection(const Vector2<T>& aPoint, const Vector2<T>& aDirection);

		// Init the line with a point and a direction.
		void InitWithPointAndNormal(const Vector2<T>& aPoint, const Vector2<T>& aNormal);

		// Returns whether a point is inside the line: it is inside when the point
		// is on the line or on the side the normal is pointing away from.
		// Always returns false if used with an uninitialized line
		bool IsInside(const Vector2<T>& aPosition) const;

		// Returns the direction of the line, which is (normal.y, -normal.x).
		// If saved as member variable return as const reference
		Vector2<T> GetDirection() const;

		// Returns the normal of the line, which is (-direction.y, direction.x).
		// If saved as member variable return as const reference
		Vector2<T> GetNormal() const;

	private:
		static Vector2<T> ComputeNormalFromDirection(const Vector2<T>& aDirection);

		bool IsInitialized() const;

		Vector2<T> myPoint;

		Vector2<T> myNormal;
	};

	template <typename T>
	inline Line<T>::Line()
		: myPoint(T(0), T(0))
		, myNormal(T(0), T(0)) // zero normal = uninitialized
	{
	}

	template <typename T>
	inline Line<T>::Line(const Vector2<T>& aPoint0, const Vector2<T>& aPoint1)
		: myPoint(T(0), T(0))
		, myNormal(T(0), T(0))
	{
		InitWith2Points(aPoint0, aPoint1);
	}

	template <typename T>
	inline void Line<T>::InitWith2Points(const Vector2<T>& aPoint0, const Vector2<T>& aPoint1)
	{
		const Vector2<T> direction = aPoint1 - aPoint0;

		if (direction.LengthSqr() == 0)
		{
			myPoint = Vector2<T>(T(0), T(0));
			myNormal = Vector2<T>(T(0), T(0));
			return;
		}

		myPoint = aPoint0;
		myNormal = ComputeNormalFromDirection(direction);
	}
	
	template <typename T>
	inline void Line<T>::InitWithPointAndDirection(const Vector2<T>& aPoint, const Vector2<T>& aDirection)
	{
		if (aDirection.LengthSqr() == 0)
		{
			myPoint = Vector2<T>(T(0), T(0));
			myNormal = Vector2<T>(T(0), T(0));
			return;
		}
		
		myPoint = aPoint;
		myNormal = ComputeNormalFromDirection(aDirection);
	}

	template<typename T>
	inline void Line<T>::InitWithPointAndNormal(const Vector2<T>& aPoint, const Vector2<T>& aNormal)
	{
		if (aNormal.LengthSqr() == 0)
		{
			myPoint = Vector2<T>(T(0), T(0));
			myNormal = Vector2<T>(T(0), T(0));
			return;
		}

		myPoint = aPoint;
		myNormal = aNormal.GetNormalized();
	}

	template<typename T>
	inline bool Line<T>::IsInside(const Vector2<T>& aPosition) const
	{
		if (!IsInitialized())
		{
			return false;
		}

		// Vector from a point on the line to the test point
		const Vector2<T> offset = aPosition - myPoint;

		// Dot > 0 -> in direction of normal -> outside
		// Dot <= 0 -> on line or behind normal -> inside
		const T dot = offset.Dot(myNormal);
		return dot <= T(0);
	}

	template<typename T>
	inline Vector2<T> Line<T>::GetDirection() const
	{
		return Vector2<T>(myNormal.y, -myNormal.x);
	}

	template<typename T>
	inline Vector2<T> Line<T>::GetNormal() const
	{
		return myNormal;
	}

	// Private helpers
	template<typename T>
	inline Vector2<T> Line<T>::ComputeNormalFromDirection(const Vector2<T>& aDirection)
	{
		// Perpendicular (rotate 90 degrees CCW) and normalize
		Vector2<T> n(-aDirection.y, aDirection.x);
		const T lenSqr = n.LengthSqr();
		if (lenSqr == T(0))
		{
			return Vector2<T>(T(0), T(0));
		}
		const T invLen = static_cast<T>(1) / static_cast<T>(std::sqrt(lenSqr));
		return Vector2<T>(n.x * invLen, n.y * invLen);
	}

	template<typename T>
	inline bool Line<T>::IsInitialized() const
	{
		return myNormal.LengthSqr() != T(0);
	}
} // namespace CommonUtilities





