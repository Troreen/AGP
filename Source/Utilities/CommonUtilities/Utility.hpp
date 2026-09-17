#pragma once

#include "Vector.hpp"

#include <assert.h>
#include <cmath>
#include <numbers>
#include <type_traits>

namespace CommonUtilities::Math
{
    template<class T>
    constexpr T EPSILON = static_cast<T>(1E-7f);

    template<class T>
    constexpr T DEGREES_TO_RADIANS = static_cast<T>(std::numbers::pi / 180.0);
    template<class T>
    constexpr T RADIANS_TO_DEGREES = static_cast<T>(180.0 / std::numbers::pi);

    template<typename T>
    static constexpr bool NumberInRange(T value, T min, T max)
    {
        return value >= min && value <= max;
    }

    template <typename T> 
    int Sign(T val)
    {
        return (T(0) < val) - (val < T(0));
    }

    template<typename T>
    T Max(const T& aFirst, const T& aSecond)
    {
        return aFirst < aSecond ? aSecond : aFirst;
    }

    template<typename T>
    T Min(const T& aFirst, const T& aSecond)
    {
        return aFirst < aSecond ? aFirst : aSecond;
    }

    template<typename T>
    T Abs(const T& aValue)
    {
        return aValue < 0 ? -aValue: aValue;
    }

    template<typename T>
    T Clamp(const T& aValue, const T& aMin, const T& aMax)
    {
        assert((aMin < aMax || !(aMax < aMin)) && "The given minimum value must be less than or equal to the maximum.");

        return aValue < aMin ? aMin : (aMax < aValue ? aMax : aValue);
    }

    template<typename T>
    T Clamp01(const T& aValue)
    {
        constexpr T zero = static_cast<T>(0);
        constexpr T one = static_cast<T>(1);
        return aValue < zero ? zero : (one < aValue ? one : aValue);
    }

    template<typename T>
    T Lerp(const T& aStart, const T& aEnd, float aT)
    {
        aT = Clamp(aT, 0.0f, 1.0f);
        return static_cast<T>(aStart + ((aEnd - aStart) * aT));
    }

    template<typename T>
    void Swap(T& aFirst, T& aSecond)
    {
        T copy(std::move(aFirst));
        aFirst = std::move(aSecond);
        aSecond = std::move(copy);
    }

    template<typename T>
    T Repeat(const T& t, const T& aLength)
    {
        return Clamp<T>(t - floor(t / aLength) * aLength, static_cast<T>(0), aLength);
    }

    template<typename T>
    T DeltaAngle(const T& aCurrent, const T& aTarget)
    {
        const T fullRotation = static_cast<T>(360);
        const T halfRotation = static_cast<T>(180);
        T delta = Repeat<T>(aTarget - aCurrent, fullRotation);
        return delta > halfRotation ? delta - fullRotation : delta;
    }

    template<typename T>
    T VectorAngle(const Vector3<T>& aFrom, const Vector3<T>& aTo)
    {
        T denominator = sqrt(aFrom.LengthSqr() * aTo.LengthSqr());
        if (denominator < EPSILON<T>)
        {
            return static_cast<T>(0);
        }

        T dot = Clamp(aFrom.Dot(aTo) / denominator, static_cast<T>(-1), static_cast<T>(1));
        return acos(dot) * RADIANS_TO_DEGREES<T>;
    }

    template<typename T>
    Vector3<T> Slerp(const Vector3<T>& aStart, const Vector3<T>& aEnd, float aT)
    {
        constexpr T zero = static_cast<T>(0);
        constexpr T one = static_cast<T>(1);
        constexpr T oneNeg = static_cast<T>(-1);

        T dot = Clamp(aStart.Dot(aEnd), oneNeg, one);

        T theta = acos(dot) * Clamp(aT, zero, one);
        Vector3<T> relative = aEnd - (aStart * dot);
        relative.Normalize();

        return (aStart * cos(theta)) + (relative * sin(theta));
    }

    template<typename T>
    Vector2<T> LerpVector(const Vector2<T>& aStart, const Vector2<T>& aEnd, float aT)
    {
        return Vector2<T>(Lerp(aStart.x, aEnd.x, aT), Lerp(aStart.y, aEnd.y, aT));
    }

    template<typename T>
    Vector3<T> LerpVector(const Vector3<T>& aStart, const Vector3<T>& aEnd, float aT)
    {
        return Vector3<T>(Lerp(aStart.x, aEnd.x, aT), Lerp(aStart.y, aEnd.y, aT), Lerp(aStart.z, aEnd.z, aT));
    }

    template<typename T>
    Vector4<T> LerpVector(const Vector4<T>& aStart, const Vector4<T>& aEnd, float aT)
    {
        return Vector4<T>(Lerp(aStart.x, aEnd.x, aT), Lerp(aStart.y, aEnd.y, aT), Lerp(aStart.z, aEnd.z, aT), Lerp(aStart.w, aEnd.w, aT));
    }
}
