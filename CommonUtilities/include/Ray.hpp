#pragma once
#include "Vector3.hpp"

namespace CommonUtilities
{
    template <typename T>
    class Ray
    {
    public:
        // Default constructor: there is no ray, the origin and direction are the
        // zero vector.
        Ray();
 
        // Constructor that takes origin and direction to define the ray
        Ray(const Vector3<T>& aOrigin, const Vector3<T>& aDirection);
 
        // Init the ray with two points.
        void InitWith2Points(const Vector3<T>& aOrigin, const Vector3<T>& aPoint);
 
        // Init the ray with an origin and a direction.
        void InitWithOriginAndDirection(const Vector3<T>& aOrigin, const Vector3<T>& aDirection);
 
        // Returns the normalized direction of the ray.
        const Vector3<T>& GetDirection() const;
 
        // Returns the origin of the ray.
        const Vector3<T>& GetOrigin() const;

        // Returns whether a point is in front of the line: it is in front of when the point
        // is on the side of the point in the direction’s direction or is perpendicular to it.
        // Uninitialized rays should always return false
        bool IsInFront(const Vector3<T>& aPosition) const;

    private:
        Vector3<T> myOrigin;
        Vector3<T> myDirection;
    };

    template <typename T>
    inline Ray<T>::Ray()
        : myOrigin(Vector3<T>::Zero)
        , myDirection(Vector3<T>::Zero)
    {
    }

    template <typename T>
    inline Ray<T>::Ray(const Vector3<T>& aOrigin, const Vector3<T>& aDirection)
        : myOrigin(aOrigin)
        , myDirection(aDirection.GetNormalized())
    {
    }

    template <typename T>
    inline void Ray<T>::InitWith2Points(const Vector3<T>& aOrigin, const Vector3<T>& aPoint)
    {
        myOrigin = aOrigin;
        myDirection = (aPoint - aOrigin).GetNormalized();
    }

    template <typename T>
    inline void Ray<T>::InitWithOriginAndDirection(const Vector3<T>& aOrigin, const Vector3<T>& aDirection)
    {
        myOrigin = aOrigin;
        myDirection = aDirection.GetNormalized();
    }

    template <typename T>
    inline const Vector3<T>& Ray<T>::GetDirection() const
    {
        return myDirection;
    }

    template <typename T>
    inline const Vector3<T>& Ray<T>::GetOrigin() const
    {
        return myOrigin;
    }

    template <typename T>
    inline bool Ray<T>::IsInFront(const Vector3<T>& aPosition) const
    {
        if (myDirection.LengthSqr() == static_cast<T>(0))
        {
            return false;
        }
    
        Vector3<T> toPoint = aPosition - myOrigin;
        T dotProduct = myDirection.Dot(toPoint);
        return dotProduct >= static_cast<T>(0);
    }

} // namespace CommonUtilities
