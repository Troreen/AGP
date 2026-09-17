#pragma once
#include <cmath>
#include <limits>
#include <utility>
#include <type_traits>
#include "Plane.hpp"
#include "Ray.hpp"
#include "Sphere.hpp"
#include "AABB3D.hpp"

namespace CommonUtilities
{
    template <typename T>
    bool IntersectionPlaneRay(const Plane<T>& aPlane, const Ray<T>& aRay);

    // If the ray is parallel to the plane, aOutIntersectionPoint remains unchanged. If
    // the ray is in the plane, true is returned, if not, false is returned. If the ray
    // isn't parallel to the plane and hits, the intersection point is stored in
    // aOutIntersectionPoint and true returned otherwise false is returned and
    // outIntersectionPoint is unchanged.
    template <typename T>
    bool IntersectionPlaneRay(const Plane<T>& aPlane, const Ray<T>& aRay, Vector3<T>& outIntersectionPoint);

    // If no collision, aOutIntersectionPoint remains unchanged.
    // If The sphere overlaps the AABB true is returned, if not, false is returned.
    // The point on the AABB closest to the sphere centre is saved in
    // aOutIntersectionPoint.
    // A sphere touching the aabb is considered overlapping.
    template <typename T>
    bool IntersectionSphereAABB(const Sphere<T>& aSphere, const AABB3D<T>& aAABB3D);

    template <typename T>
    bool IntersectionSphereAABB(const Sphere<T>& aSphere, const AABB3D<T>& aAABB3D, Vector3<T>& outIntersectionPoint);

    // If the ray intersects the AABB, true is returned, if not, false is returned.
    // A ray in one of the AABB's sides is counted as intersecting it.
    template <typename T>
    bool IntersectionAABBRay(const AABB3D<T>& aAABB, const Ray<T>& aRay);

    template <typename T>
    bool IntersectionAABBRay(const AABB3D<T>& aAABB, const Ray<T>& aRay, Vector3<T>& outIntersectionPoint);

    // If the ray intersects the sphere, true is returned, if not, false is returned.
    // A ray intersecting the surface of the sphere is considered as intersecting it.
    template <typename T>
    bool IntersectionSphereRay(const Sphere<T>& aSphere, const Ray<T>& aRay);

    template <typename T>
    bool IntersectionSphereRay(const Sphere<T>& aSphere, const Ray<T>& aRay, Vector3<T>& outIntersectionPoint);

    // If the plane intersects the sphere, true is returned, otherwise false.
    // When intersecting (including touching), the closest point on the plane to the sphere center
    // is written to outIntersectionPoint.
    template <typename T>
    bool IntersectionPlaneSphere(const Plane<T>& aPlane, const Sphere<T>& aSphere);

    template <typename T>
    bool IntersectionPlaneSphere(const Plane<T>& aPlane, const Sphere<T>& aSphere, Vector3<T>& outIntersectionPoint);

    // If the plane intersects the AABB, true is returned, otherwise false.
    // When intersecting (including touching), the projection of the AABB center onto the plane
    // is written to outIntersectionPoint.
    template <typename T>
    bool IntersectionPlaneAABB(const Plane<T>& aPlane, const AABB3D<T>& aAABB);

    template <typename T>
    bool IntersectionPlaneAABB(const Plane<T>& aPlane, const AABB3D<T>& aAABB, Vector3<T>& outIntersectionPoint);
    // Returns true if two spheres overlap or touch.
    template <typename T>
    bool IntersectionSphereSphere(const Sphere<T>& aLeft, const Sphere<T>& aRight);

    template <typename T>
    bool IntersectionSphereSphere(const Sphere<T>& aLeft, const Sphere<T>& aRight, Vector3<T>& outIntersectionPoint);

    // Returns true if two AABBs overlap or touch.
    template <typename T>
    bool IntersectionAABBAABB(const AABB3D<T>& aLeft, const AABB3D<T>& aRight);

    template <typename T>
    bool IntersectionAABBAABB(const AABB3D<T>& aLeft, const AABB3D<T>& aRight, Vector3<T>& outIntersectionPoint);

    //=====================================
    //  Implementations below this line
    //=====================================

    template <typename T>
    bool IntersectionPlaneRay(const Plane<T>& aPlane, const Ray<T>& aRay)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& O = aRay.GetOrigin();
        const Vector3<T>& D = aRay.GetDirection();
        const Vector3<T>& n = aPlane.GetNormal();

        const T zero = static_cast<T>(0);
        const T eps = static_cast<T>(1e-6);

        if (n.LengthSqr() <= eps * eps)
        {
            return false;
        }
        const T sd = aPlane.GetDistanceToPoint(O);

        if (D.LengthSqr() <= eps * eps)
        {
            return std::abs(sd) <= eps;
        }

        const T denom = n.Dot(D);

        if (std::abs(denom) <= eps)
        {
            return std::abs(sd) <= eps;
        }

        const T t = -sd / denom;

        if (t < -eps)
        {
            return false;
        }

        return true;
    }

    template <typename T>
    bool IntersectionPlaneRay(const Plane<T>& aPlane, const Ray<T>& aRay, Vector3<T>& outIntersectionPoint)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& O = aRay.GetOrigin();
        const Vector3<T>& D = aRay.GetDirection();
        const Vector3<T>& n = aPlane.GetNormal();

        const T eps = static_cast<T>(1e-6);

        if (n.LengthSqr() <= eps * eps)
            return false;

        const T sd = aPlane.GetDistanceToPoint(O);

        if (D.LengthSqr() <= eps * eps)
            return std::abs(sd) <= eps;

        const T denom = n.Dot(D);

        if (std::abs(denom) <= eps)
            return std::abs(sd) <= eps;

        const T t = -sd / denom;

        if (t < -eps)
            return false;

        outIntersectionPoint = O + D * t;
        return true;
    }

    template <typename T>
    bool IntersectionSphereAABB(const Sphere<T>& aSphere, const AABB3D<T>& aAABB3D)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& center = aSphere.GetCenter();
        const T radius = aSphere.GetRadius();

        const Vector3<T>& min = aAABB3D.GetMin();
        const Vector3<T>& max = aAABB3D.GetMax();

        Vector3<T> closest;
        if (center.x < min.x) closest.x = min.x;
        else if (center.x > max.x) closest.x = max.x;
        else closest.x = center.x;

        if (center.y < min.y) closest.y = min.y;
        else if (center.y > max.y) closest.y = max.y;
        else closest.y = center.y;

        if (center.z < min.z) closest.z = min.z;
        else if (center.z > max.z) closest.z = max.z;
        else closest.z = center.z;

        const Vector3<T> diff = closest - center;
        const T distSq = diff.LengthSqr();
        const T radiusSq = radius * radius;

        return distSq <= radiusSq;
    }

    template <typename T>
    bool IntersectionSphereAABB(const Sphere<T>& aSphere, const AABB3D<T>& aAABB3D, Vector3<T>& outIntersectionPoint)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& center = aSphere.GetCenter();
        const T radius = aSphere.GetRadius();

        const Vector3<T>& min = aAABB3D.GetMin();
        const Vector3<T>& max = aAABB3D.GetMax();

        Vector3<T> closest;
        // X axis
        if (center.x < min.x) closest.x = min.x;
        else if (center.x > max.x) closest.x = max.x;
        else closest.x = center.x;
        // Y axis
        if (center.y < min.y) closest.y = min.y;
        else if (center.y > max.y) closest.y = max.y;
        else closest.y = center.y;
        // Z axis
        if (center.z < min.z) closest.z = min.z;
        else if (center.z > max.z) closest.z = max.z;
        else closest.z = center.z;

        const Vector3<T> diff = closest - center;
        const T distSq = diff.LengthSqr();
        const T radiusSq = radius * radius;

        if (distSq <= radiusSq)
        {
            outIntersectionPoint = closest;
            return true;
        }

        return false;
    }

    template <typename T>
    bool IntersectionAABBRay(const AABB3D<T>& aAABB, const Ray<T>& aRay)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& min = aAABB.GetMin();
        const Vector3<T>& max = aAABB.GetMax();
        const Vector3<T>& o = aRay.GetOrigin();
        const Vector3<T>& d = aRay.GetDirection();

        const T eps = static_cast<T>(1e-6);

        T tEnter = std::numeric_limits<T>::lowest();
        T tExit = std::numeric_limits<T>::max();

        auto processAxis = [&](T origin, T dir, T minVal, T maxVal) -> bool
            {
                if (std::abs(dir) <= eps)
                {
                    return (origin >= minVal && origin <= maxVal);
                }
                T t0 = (minVal - origin) / dir;
                T t1 = (maxVal - origin) / dir;
                if (t0 > t1) { std::swap(t0, t1); }
                if (t0 > tEnter) tEnter = t0;
                if (t1 < tExit)  tExit = t1;
                return tExit >= tEnter;
            };

        if (!processAxis(o.x, d.x, min.x, max.x)) return false;
        if (!processAxis(o.y, d.y, min.y, max.y)) return false;
        if (!processAxis(o.z, d.z, min.z, max.z)) return false;

        if (tExit < -eps)
        {
            return false;
        }

        return true;
    }

    template <typename T>
    bool IntersectionAABBRay(const AABB3D<T>& aAABB, const Ray<T>& aRay, Vector3<T>& outIntersectionPoint)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& min = aAABB.GetMin();
        const Vector3<T>& max = aAABB.GetMax();
        const Vector3<T>& o = aRay.GetOrigin();
        const Vector3<T>& d = aRay.GetDirection();

        const T zero = static_cast<T>(0);
        const T eps = static_cast<T>(1e-6);

        T tEnter = std::numeric_limits<T>::lowest();
        T tExit = std::numeric_limits<T>::max();

        auto processAxis = [&](T origin, T dir, T minVal, T maxVal) -> bool
            {
                if (std::abs(dir) <= eps)
                {
                    return (origin >= minVal && origin <= maxVal);
                }
                T t0 = (minVal - origin) / dir;
                T t1 = (maxVal - origin) / dir;
                if (t0 > t1) { std::swap(t0, t1); }
                if (t0 > tEnter) tEnter = t0;
                if (t1 < tExit)  tExit = t1;
                return tExit >= tEnter;
            };

        if (!processAxis(o.x, d.x, min.x, max.x)) return false;
        if (!processAxis(o.y, d.y, min.y, max.y)) return false;
        if (!processAxis(o.z, d.z, min.z, max.z)) return false;

        if (tExit < -eps)
        {
            return false;
        }

        const T tHit = (tEnter >= -eps) ? tEnter : tExit;
        outIntersectionPoint = o + d * tHit;
        return true;
    }

    template <typename T>
    bool IntersectionPlaneSphere(const Plane<T>& aPlane, const Sphere<T>& aSphere)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& n = aPlane.GetNormal();
        const T eps = static_cast<T>(1e-6);
        if (n.LengthSqr() <= eps * eps)
        {
            return false;
        }

        const Vector3<T>& c = aSphere.GetCenter();
        const T r = aSphere.GetRadius();
        const T dist = aPlane.GetDistanceToPoint(c);
        return std::abs(dist) <= r + eps;
    }

    template <typename T>
    bool IntersectionPlaneSphere(const Plane<T>& aPlane, const Sphere<T>& aSphere, Vector3<T>& outIntersectionPoint)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& n = aPlane.GetNormal();
        const T zero = static_cast<T>(0);
        const T eps = static_cast<T>(1e-6);
        if (n.LengthSqr() <= eps * eps)
        {
            return false;
        }

        const Vector3<T>& c = aSphere.GetCenter();
        const T r = aSphere.GetRadius();
        const T dist = aPlane.GetDistanceToPoint(c);
        if (std::abs(dist) <= r + eps)
        {
            outIntersectionPoint = c - n * dist;
            return true;
        }
        return false;
    }

    template <typename T>
    bool IntersectionPlaneAABB(const Plane<T>& aPlane, const AABB3D<T>& aAABB)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& n = aPlane.GetNormal();
        const T eps = static_cast<T>(1e-6);
        if (n.LengthSqr() <= eps * eps)
        {
            return false;
        }

        const Vector3<T>& min = aAABB.GetMin();
        const Vector3<T>& max = aAABB.GetMax();
        const T half = static_cast<T>(0.5);
        const Vector3<T> center = (min + max) * half;
        const Vector3<T> extents = (max - min) * half;

        const T r = std::abs(n.x) * extents.x + std::abs(n.y) * extents.y + std::abs(n.z) * extents.z;
        const T s = aPlane.GetDistanceToPoint(center);

        return std::abs(s) <= r + eps;
    }

    template <typename T>
    bool IntersectionPlaneAABB(const Plane<T>& aPlane, const AABB3D<T>& aAABB, Vector3<T>& outIntersectionPoint)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& n = aPlane.GetNormal();
        const T zero = static_cast<T>(0);
        const T eps = static_cast<T>(1e-6);
        if (n.LengthSqr() <= eps * eps)
        {
            return false;
        }

        const Vector3<T>& min = aAABB.GetMin();
        const Vector3<T>& max = aAABB.GetMax();
        const T half = static_cast<T>(0.5);
        const Vector3<T> center = (min + max) * half;
        const Vector3<T> extents = (max - min) * half;

        const T r = std::abs(n.x) * extents.x + std::abs(n.y) * extents.y + std::abs(n.z) * extents.z;
        const T s = aPlane.GetDistanceToPoint(center);

        if (std::abs(s) <= r + eps)
        {
            outIntersectionPoint = center - n * s;
            return true;
        }
        return false;
    }

    template <typename T>
    bool IntersectionSphereRay(const Sphere<T>& aSphere, const Ray<T>& aRay)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& center = aSphere.GetCenter();
        const T radius = aSphere.GetRadius();
        const Vector3<T>& origin = aRay.GetOrigin();
        const Vector3<T>& dir = aRay.GetDirection();

        const T zero = static_cast<T>(0);
        const T eps = static_cast<T>(1e-6);
        if (dir.LengthSqr() <= eps * eps)
        {
            return false;
        }
        const Vector3<T> m = origin - center;
        const T a = dir.Dot(dir);
        if (std::abs(a) <= eps) { return false; }
        const T bHalf = m.Dot(dir);
        const T c = m.Dot(m) - radius * radius;
        const T disc = bHalf * bHalf - a * c;
        if (disc < -eps) { return false; }
        const T clampedDisc = (disc < zero) ? zero : disc;
        const T sqrtDisc = static_cast<T>(std::sqrt(clampedDisc));
        T t0 = (-bHalf - sqrtDisc) / a;
        T t1 = (-bHalf + sqrtDisc) / a;
        if (t0 > t1) { std::swap(t0, t1); }
        if (t0 >= -eps)
        {
            return true;
        }
        if (t1 >= -eps)
        {
            return true;
        }
        return false;
    }

    template <typename T>
    bool IntersectionSphereRay(const Sphere<T>& aSphere, const Ray<T>& aRay, Vector3<T>& outIntersectionPoint)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& center = aSphere.GetCenter();
        const T radius = aSphere.GetRadius();
        const Vector3<T>& origin = aRay.GetOrigin();
        const Vector3<T>& dir = aRay.GetDirection();

        const T zero = static_cast<T>(0);
        const T eps = static_cast<T>(1e-6);
        if (dir.LengthSqr() <= eps * eps)
        {
            return false;
        }
        const Vector3<T> m = origin - center;
        const T a = dir.Dot(dir);
        if (std::abs(a) <= eps) { return false; }
        const T bHalf = m.Dot(dir);
        const T c = m.Dot(m) - radius * radius;
        const T disc = bHalf * bHalf - a * c;
        if (disc < -eps) { return false; }
        const T clampedDisc = (disc < zero) ? zero : disc;
        const T sqrtDisc = static_cast<T>(std::sqrt(clampedDisc));
        T t0 = (-bHalf - sqrtDisc) / a;
        T t1 = (-bHalf + sqrtDisc) / a;
        if (t0 > t1) { std::swap(t0, t1); }
        if (t0 >= -eps)
        {
            outIntersectionPoint = origin + dir * t0;
            return true;
        }
        if (t1 >= -eps)
        {
            outIntersectionPoint = origin + dir * t1;
            return true;
        }
        return false;
    }

    template <typename T>
    bool IntersectionSphereSphere(const Sphere<T>& aLeft, const Sphere<T>& aRight)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& cA = aLeft.GetCenter();
        const Vector3<T>& cB = aRight.GetCenter();
        const Vector3<T> diff = cB - cA;
        const T distSq = diff.LengthSqr();
        const T radiusSum = aLeft.GetRadius() + aRight.GetRadius();
        const T radiusSumSq = radiusSum * radiusSum;
        return distSq <= radiusSumSq;
    }

    template <typename T>
    bool IntersectionSphereSphere(const Sphere<T>& aLeft, const Sphere<T>& aRight, Vector3<T>& outIntersectionPoint)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& cA = aLeft.GetCenter();
        const Vector3<T>& cB = aRight.GetCenter();
        const Vector3<T> diff = cB - cA;
        const T distSq = diff.LengthSqr();
        const T radiusSum = aLeft.GetRadius() + aRight.GetRadius();
        const T radiusSumSq = radiusSum * radiusSum;
        if (distSq > radiusSumSq)
        {
            return false;
        }
        const T eps = static_cast<T>(1e-6);
        const T dist = static_cast<T>(std::sqrt(distSq));
        if (dist <= eps)
        {
            outIntersectionPoint = cA;
            return true;
        }
        const Vector3<T> n = diff / dist;
        outIntersectionPoint = cA + n * aLeft.GetRadius();
        return true;
    }

    template <typename T>
    bool IntersectionAABBAABB(const AABB3D<T>& aLeft, const AABB3D<T>& aRight)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& minA = aLeft.GetMin();
        const Vector3<T>& maxA = aLeft.GetMax();
        const Vector3<T>& minB = aRight.GetMin();
        const Vector3<T>& maxB = aRight.GetMax();

        const T overlapMinX = (minA.x > minB.x) ? minA.x : minB.x;
        const T overlapMinY = (minA.y > minB.y) ? minA.y : minB.y;
        const T overlapMinZ = (minA.z > minB.z) ? minA.z : minB.z;
        const T overlapMaxX = (maxA.x < maxB.x) ? maxA.x : maxB.x;
        const T overlapMaxY = (maxA.y < maxB.y) ? maxA.y : maxB.y;
        const T overlapMaxZ = (maxA.z < maxB.z) ? maxA.z : maxB.z;

        const bool overlapX = (overlapMinX <= overlapMaxX);
        const bool overlapY = (overlapMinY <= overlapMaxY);
        const bool overlapZ = (overlapMinZ <= overlapMaxZ);

        return overlapX && overlapY && overlapZ;
    }

    template <typename T>
    bool IntersectionAABBAABB(const AABB3D<T>& aLeft, const AABB3D<T>& aRight, Vector3<T>& outIntersectionPoint)
    {
        static_assert(std::is_floating_point<T>::value, "Intersection requires floating-point T");
        const Vector3<T>& minA = aLeft.GetMin();
        const Vector3<T>& maxA = aLeft.GetMax();
        const Vector3<T>& minB = aRight.GetMin();
        const Vector3<T>& maxB = aRight.GetMax();

        const T overlapMinX = (minA.x > minB.x) ? minA.x : minB.x;
        const T overlapMinY = (minA.y > minB.y) ? minA.y : minB.y;
        const T overlapMinZ = (minA.z > minB.z) ? minA.z : minB.z;
        const T overlapMaxX = (maxA.x < maxB.x) ? maxA.x : maxB.x;
        const T overlapMaxY = (maxA.y < maxB.y) ? maxA.y : maxB.y;
        const T overlapMaxZ = (maxA.z < maxB.z) ? maxA.z : maxB.z;

        const bool overlapX = (overlapMinX <= overlapMaxX);
        const bool overlapY = (overlapMinY <= overlapMaxY);
        const bool overlapZ = (overlapMinZ <= overlapMaxZ);

        if (!(overlapX && overlapY && overlapZ))
        {
            return false;
        }

        outIntersectionPoint = Vector3<T>(
            (overlapMinX + overlapMaxX) / static_cast<T>(2),
            (overlapMinY + overlapMaxY) / static_cast<T>(2),
            (overlapMinZ + overlapMaxZ) / static_cast<T>(2));
        return true;
    }
}