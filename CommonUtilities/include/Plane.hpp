#pragma once
#include "Vector3.hpp"


namespace CommonUtilities
{
	template <typename T>
	class Plane
	{
	public:
		// Default constructor.
		Plane();

		// Constructor taking three points where the normal is (aPoint1 - aPoint0) x (aPoint2-aPoint0).
		Plane(const Vector3<T>& aPoint0, const Vector3<T>& aPoint1, const Vector3<T>& aPoint2);

		// Constructor taking a point and a normal.
		Plane(const Vector3<T>& aPoint0, const Vector3<T>& aNormal);

		// Init the plane with three points, the same as the constructor above.
		void InitWith3Points(const Vector3<T>& aPoint0, const Vector3<T>& aPoint1, const Vector3<T>& aPoint2);

		// Init the plane with a point and a normal, the same as the constructor above.
		void InitWithPointAndNormal(const Vector3<T>& aPoint, const Vector3<T>& aNormal);

		// Returns whether a point is inside the plane: it is inside when the point is on the plane or on the side the normal is pointing away from.
		// Always returns false if used with an uninitialized line
		bool IsInside(const Vector3<T>& aPosition) const;

		// Returns the normal of the plane.
		const Vector3<T>& GetNormal() const;

		// Returns the signed distance from a point to the plane.
		// Negative or zero means the point is on the plane or on the side the normal points away from.
		T GetDistanceToPoint(const Vector3<T>& aPosition) const;

	private:
		Vector3<T> myNormal;
		T myD; // plane equation: n dot x + d = 0
		bool myInitialized;
	};

	//================
	// Implementations
	//================
	template <typename T>
	inline Plane<T>::Plane()
		: myNormal(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))
		, myD(static_cast<T>(0))
		, myInitialized(false)
	{
	}

	template <typename T>
	inline Plane<T>::Plane(const Vector3<T>& aPoint0, const Vector3<T>& aPoint1, const Vector3<T>& aPoint2)
		: myNormal(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))
		, myD(static_cast<T>(0))
		, myInitialized(false)
	{
		InitWith3Points(aPoint0, aPoint1, aPoint2);
	}

	template <typename T>
	inline Plane<T>::Plane(const Vector3<T>& aPoint0, const Vector3<T>& aNormal)
		: myNormal(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))
		, myD(static_cast<T>(0))
		, myInitialized(false)
	{
		InitWithPointAndNormal(aPoint0, aNormal);
	}

	template <typename T>
	inline void Plane<T>::InitWith3Points(const Vector3<T>& aPoint0, const Vector3<T>& aPoint1, const Vector3<T>& aPoint2)
	{
		const Vector3<T> v1 = aPoint1 - aPoint0;
		const Vector3<T> v2 = aPoint2 - aPoint0;
		myNormal = v1.Cross(v2).GetNormalized();

		const T zero = static_cast<T>(0);
		if (myNormal.LengthSqr() == zero)
		{
			myInitialized = false;
			myD = zero;
			return;
		}
		// d = -n dot P0
		myD = -myNormal.Dot(aPoint0);
		myInitialized = true;
	}

	template <typename T>
	inline void Plane<T>::InitWithPointAndNormal(const Vector3<T>& aPoint, const Vector3<T>& aNormal)
	{
		const T zero = static_cast<T>(0);
		// Ensure normal is normalized for consistent distance results
		Vector3<T> n = aNormal.GetNormalized();
		if (n.LengthSqr() == zero)
		{
			myInitialized = false;
			myNormal = Vector3<T>::Zero;
			myD = zero;
			return;
		}
		myNormal = n;
		myD = -myNormal.Dot(aPoint);
		myInitialized = true;
	}

	template <typename T>
	inline bool Plane<T>::IsInside(const Vector3<T>& aPosition) const
	{
		if (!myInitialized)
		{
			return false;
		}
		// Inside means on plane or on the side the normal points away from: distance <= 0
		return GetDistanceToPoint(aPosition) <= static_cast<T>(0);
	}

	template <typename T>
	inline const Vector3<T>& Plane<T>::GetNormal() const
	{
		return myNormal;
	}

	template <typename T>
	inline T Plane<T>::GetDistanceToPoint(const Vector3<T>& aPosition) const
	{
		// Signed distance using plane equation n·x + d. Assumes n is normalized.
		return myNormal.Dot(aPosition) + myD;
	}
}