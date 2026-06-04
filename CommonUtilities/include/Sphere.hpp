#pragma once
#include "Vector3.hpp"

namespace CommonUtilities
{
    template <typename T>
    class Sphere
    {
    public:
        // Default constructor: there is no sphere, the radius is zero and the position is
        // the zero vector.
        Sphere();
        
        // Constructor that takes the center position and radius of the sphere.
        Sphere(const Vector3<T>& aCenter, T aRadius);
        
        // Init the sphere with a center and a radius, the same as the constructor above.
        void InitWithCenterAndRadius(const Vector3<T>& aCenter, T aRadius);
        
        // Returns the center of the sphere.
        const Vector3<T>& GetCenter() const;
        
        // Returns the radius of the sphere.
        T GetRadius() const;
        
        // Returns whether a point is inside the sphere: it is inside when the point is on the
        // sphere surface or inside of the sphere.
        // Uninitiallized should always return false
        bool IsInside(const Vector3<T>& aPosition) const;

    private:
        Vector3<T> myCenter;
        T myRadius;
    };

    template <typename T>
    inline Sphere<T>::Sphere()
        : myCenter()
        , myRadius(static_cast<T>(0))
    {
    }

    template <typename T>
    inline Sphere<T>::Sphere(const Vector3<T>& aCenter, T aRadius)
        : myCenter(aCenter)
        , myRadius(aRadius)
    {
    }

    template <typename T>
    inline void Sphere<T>::InitWithCenterAndRadius(const Vector3<T>& aCenter, T aRadius)
    {
        myCenter = aCenter;
        myRadius = aRadius;
    }

    template <typename T>
    inline const Vector3<T>& Sphere<T>::GetCenter() const
    {
        return myCenter;
    }

    template <typename T>
    inline T Sphere<T>::GetRadius() const
    {
        return myRadius;
    }

    template <typename T>
    inline bool Sphere<T>::IsInside(const Vector3<T>& aPosition) const
    {
        if (myRadius <= static_cast<T>(0))
        {
            return false;
        }

        Vector3<T> diff = aPosition - myCenter;

        T distanceSquared = diff.LengthSqr();
        T radiusSquared = myRadius * myRadius;
        
        return distanceSquared <= radiusSquared;
        
    }
}