#pragma once
#include "Vector3.hpp"

namespace CommonUtilities
{
    template <typename T>
    class AABB3D
    {
    public:
        // Default constructor: there is no AABB, both min and max points are the zero vector.
        AABB3D();
        
        // Constructor taking the positions of the minimum and maximum corners.
        AABB3D(const Vector3<T>& aMin, const Vector3<T>& aMax);
       
        // Init the AABB with the positions of the minimum and maximum corners, same as
        // the constructor above.
        void InitWithMinAndMax(const Vector3<T>& aMin, const Vector3<T>& aMax);
       
        // Returns the min point of the AABB.
        const Vector3<T>& GetMin() const;
       
        // Returns the max point of the AABB.
        const Vector3<T>& GetMax() const;

        // Returns whether a point is inside the AABB: it is inside when the point is on any
        // of the AABB's sides or inside of the AABB.
        // Uninitiallized should always return false
        bool IsInside(const Vector3<T>& aPosition) const;

    private:
        Vector3<T> myMin;
        Vector3<T> myMax;
    };

    template <typename T>
    inline AABB3D<T>::AABB3D()
        : myMin(Vector3<T>::Zero)
        , myMax(Vector3<T>::Zero)
    {
    }

    template <typename T>
    inline AABB3D<T>::AABB3D(const Vector3<T>& aMin, const Vector3<T>& aMax)
        : myMin(aMin)
        , myMax(aMax)
    {
    }

    template <typename T>
    inline void AABB3D<T>::InitWithMinAndMax(const Vector3<T>& aMin, const Vector3<T>& aMax)
    {
        myMin = aMin;
        myMax = aMax;
    }

    template <typename T>
    inline const Vector3<T>& AABB3D<T>::GetMin() const
    {
        return myMin;
    }

    template <typename T>
    inline const Vector3<T>& AABB3D<T>::GetMax() const
    {
        return myMax;
    }

    template <typename T>
    inline bool AABB3D<T>::IsInside(const Vector3<T>& aPosition) const
    {
        return (aPosition.x >= myMin.x && aPosition.x <= myMax.x) &&
               (aPosition.y >= myMin.y && aPosition.y <= myMax.y) &&
               (aPosition.z >= myMin.z && aPosition.z <= myMax.z);
    }
} // namespace CommonUtilities