#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "Matrix3x3.hpp"
#include "Matrix4x4.hpp"
#include "Quaternion.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"

namespace CommonUtilities
{
	namespace Maths
	{
		// CommonUtilities uses row-vector matrix multiplication: transformed = vector * matrix.
		// The engine-facing helpers in this file assume a left-handed 3D convention with +Z as
		// forward and +Y as up, matching Camera3D and Transform. Transform matrices are composed
		// in SRT order: Scale * Rotation * Translation.

		#pragma region Constants
		template <typename T>
		constexpr T Pi()
		{
			return static_cast<T>(3.141592653589793238462643383279502884);
		}

		template <typename T>
		constexpr T TwoPi()
		{
			return Pi<T>() * static_cast<T>(2);
		}

		template <typename T>
		constexpr T HalfPi()
		{
			return Pi<T>() * static_cast<T>(0.5);
		}

		template <typename T>
		constexpr T DegToRad()
		{
			return Pi<T>() / static_cast<T>(180);
		}

		template <typename T>
		constexpr T RadToDeg()
		{
			return static_cast<T>(180) / Pi<T>();
		}

		template <typename T>
		constexpr T Epsilon()
		{
			if constexpr (std::is_floating_point_v<T>)
			{
				return static_cast<T>(0.000001);
			}
			else
			{
				return static_cast<T>(0);
			}
		}
#pragma endregion 

		#pragma region ScalarHelpers
		template <typename T>
		constexpr T Square(const T aValue)
		{
			return aValue * aValue;
		}

		template <typename T>
		constexpr T Cube(const T aValue)
		{
			return aValue * aValue * aValue;
		}

		template <typename T>
		constexpr T Abs(const T aValue)
		{
			if constexpr (std::is_unsigned_v<T>)
			{
				return aValue;
			}
			else
			{
				return aValue < static_cast<T>(0) ? -aValue : aValue;
			}
		}

		template <typename T>
		constexpr T Min(const T aA, const T aB)
		{
			return aA < aB ? aA : aB;
		}

		template <typename T>
		constexpr T Max(const T aA, const T aB)
		{
			return aA > aB ? aA : aB;
		}

		// Clamps aValue to the inclusive [aMin, aMax] range. If the range is
		// accidentally reversed, it is normalized before clamping.
		template <typename T>
		constexpr T Clamp(const T aValue, const T aMin, const T aMax)
		{
			const T low = Min(aMin, aMax);
			const T high = Max(aMin, aMax);
			return Max(low, Min(aValue, high));
		}

		template <typename T>
		constexpr T Clamp01(const T aValue)
		{
			return Clamp(aValue, static_cast<T>(0), static_cast<T>(1));
		}

		template <typename T>
		constexpr T Saturate(const T aValue)
		{
			return Clamp01(aValue);
		}

		template <typename T>
		constexpr int Sign(const T aValue)
		{
			return (static_cast<T>(0) < aValue) - (aValue < static_cast<T>(0));
		}

		template <typename T>
		constexpr T CopySign(const T aMagnitude, const T aSignSource)
		{
			return aSignSource < static_cast<T>(0) ? -Abs(aMagnitude) : Abs(aMagnitude);
		}

		template <typename T>
		inline int FloorToInt(const T aValue)
		{
			return static_cast<int>(std::floor(aValue));
		}

		template <typename T>
		inline int CeilToInt(const T aValue)
		{
			return static_cast<int>(std::ceil(aValue));
		}

		template <typename T>
		inline int RoundToInt(const T aValue)
		{
			return static_cast<int>(std::round(aValue));
		}

		template <typename T>
		inline T Fraction(const T aValue)
		{
			if constexpr (std::is_floating_point_v<T>)
			{
				return aValue - static_cast<T>(std::floor(aValue));
			}
			else
			{
				return static_cast<T>(0);
			}
		}

		// Modulo returns a non-negative result for positive divisors, unlike C++ %
		// and std::fmod which can return negative remainders for negative inputs.
		template <typename T>
		inline T Modulo(const T aValue, const T aDivisor)
		{
			if (aDivisor == static_cast<T>(0))
			{
				return static_cast<T>(0);
			}

			if constexpr (std::is_integral_v<T>)
			{
				const T result = aValue % aDivisor;
				return result < static_cast<T>(0) ? result + Abs(aDivisor) : result;
			}
			else
			{
				const T result = static_cast<T>(std::fmod(aValue, aDivisor));
				return result < static_cast<T>(0) ? result + Abs(aDivisor) : result;
			}
		}

		// Wrap maps values into [aMin, aMax). Equal min/max is treated as a fixed value.
		template <typename T>
		inline T Wrap(const T aValue, const T aMin, const T aMax)
		{
			const T size = aMax - aMin;
			if (size == static_cast<T>(0))
			{
				return aMin;
			}
			return aMin + Modulo(aValue - aMin, size);
		}

		template <typename T>
		inline T Repeat(const T aValue, const T aLength)
		{
			return Wrap(aValue, static_cast<T>(0), aLength);
		}

		// PingPong moves 0 -> length -> 0 as aValue increases, useful for simple
		// oscillating animation timers without branching in caller code.
		template <typename T>
		inline T PingPong(const T aValue, const T aLength)
		{
			if (aLength == static_cast<T>(0))
			{
				return static_cast<T>(0);
			}
			const T value = Repeat(aValue, aLength * static_cast<T>(2));
			return aLength - Abs(value - aLength);
		}
		#pragma endregion

		#pragma region NumericRobustness	
		template <typename T>
		inline bool ApproximatelyEqual(const T aA, const T aB, const T anEpsilon = Epsilon<T>())
		{
			return Abs(aA - aB) <= anEpsilon;
		}

		template <typename T>
		inline bool IsZero(const T aValue, const T anEpsilon = Epsilon<T>())
		{
			return Abs(aValue) <= anEpsilon;
		}

		template <typename T>
		inline bool IsFinite(const T aValue)
		{
			if constexpr (std::is_floating_point_v<T>)
			{
				return std::isfinite(aValue);
			}
			else
			{
				return true;
			}
		}

		template <typename T>
		constexpr bool IsPowerOfTwo(const T aValue)
		{
			static_assert(std::is_integral_v<T>, "IsPowerOfTwo requires an integral type.");
			return aValue > static_cast<T>(0) && (aValue & (aValue - static_cast<T>(1))) == static_cast<T>(0);
		}

		template <typename T>
		constexpr T NextPowerOfTwo(T aValue)
		{
			static_assert(std::is_integral_v<T>, "NextPowerOfTwo requires an integral type.");
			if (aValue <= static_cast<T>(1))
			{
				return static_cast<T>(1);
			}

			--aValue;
			for (size_t shift = 1; shift < sizeof(T) * 8; shift <<= 1)
			{
				aValue = static_cast<T>(aValue | (aValue >> shift));
			}
			return static_cast<T>(aValue + static_cast<T>(1));
		}

		template <typename T>
		constexpr T PreviousPowerOfTwo(const T aValue)
		{
			static_assert(std::is_integral_v<T>, "PreviousPowerOfTwo requires an integral type.");
			if (aValue <= static_cast<T>(0))
			{
				return static_cast<T>(0);
			}
			return static_cast<T>(NextPowerOfTwo(aValue + static_cast<T>(1)) >> 1);
		}

		template <typename T>
		constexpr T AlignUp(const T aValue, const T anAlignment)
		{
			static_assert(std::is_integral_v<T>, "AlignUp requires an integral type.");
			if (anAlignment <= static_cast<T>(0))
			{
				return aValue;
			}
			const T remainder = aValue % anAlignment;
			return remainder == static_cast<T>(0) ? aValue : static_cast<T>(aValue + anAlignment - remainder);
		}

		template <typename T>
		constexpr T AlignDown(const T aValue, const T anAlignment)
		{
			static_assert(std::is_integral_v<T>, "AlignDown requires an integral type.");
			if (anAlignment <= static_cast<T>(0))
			{
				return aValue;
			}
			return static_cast<T>(aValue - (aValue % anAlignment));
		}
		#pragma endregion

		#pragma region RangeMapping
		template <typename T>
		inline T InverseLerp(const T aStart, const T anEnd, const T aValue)
		{
			const T length = anEnd - aStart;
			if (IsZero(length))
			{
				return static_cast<T>(0);
			}
			return (aValue - aStart) / length;
		}

		template <typename T>
		inline T InverseLerpClamped(const T aStart, const T anEnd, const T aValue)
		{
			return Clamp01(InverseLerp(aStart, anEnd, aValue));
		}

		template <typename T>
		inline T Remap(const T anInputStart, const T anInputEnd, const T anOutputStart, const T anOutputEnd, const T aValue)
		{
			return anOutputStart + (anOutputEnd - anOutputStart) * InverseLerp(anInputStart, anInputEnd, aValue);
		}

		template <typename T>
		inline T RemapClamped(const T anInputStart, const T anInputEnd, const T anOutputStart, const T anOutputEnd, const T aValue)
		{
			return anOutputStart + (anOutputEnd - anOutputStart) * InverseLerpClamped(anInputStart, anInputEnd, aValue);
		}

		template <typename T>
		inline T NormalizeRange(const T aMin, const T aMax, const T aValue)
		{
			return InverseLerp(aMin, aMax, aValue);
		}
#pragma endregion

		#pragma region Angles
		template <typename T>
		constexpr T DegreesToRadians(const T aDegrees)
		{
			return aDegrees * DegToRad<T>();
		}

		template <typename T>
		constexpr T RadiansToDegrees(const T aRadians)
		{
			return aRadians * RadToDeg<T>();
		}

		// Normalizes radians into [-pi, pi), which makes shortest-angle math stable.
		template <typename T>
		inline T NormalizeRadians(const T aRadians)
		{
			return Wrap(aRadians + Pi<T>(), static_cast<T>(0), TwoPi<T>()) - Pi<T>();
		}

		template <typename T>
		inline T NormalizeDegrees(const T aDegrees)
		{
			return Wrap(aDegrees + static_cast<T>(180), static_cast<T>(0), static_cast<T>(360)) - static_cast<T>(180);
		}

		template <typename T>
		inline T DeltaAngleRadians(const T aFrom, const T aTo)
		{
			return NormalizeRadians(aTo - aFrom);
		}

		template <typename T>
		inline T DeltaAngleDegrees(const T aFrom, const T aTo)
		{
			return NormalizeDegrees(aTo - aFrom);
		}

		template <typename T>
		inline T LerpAngleRadians(const T aFrom, const T aTo, const T aT)
		{
			return aFrom + DeltaAngleRadians(aFrom, aTo) * aT;
		}

		template <typename T>
		inline T LerpAngleDegrees(const T aFrom, const T aTo, const T aT)
		{
			return aFrom + DeltaAngleDegrees(aFrom, aTo) * aT;
		}

		template <typename T>
		inline T MoveTowards(const T aCurrent, const T aTarget, const T aMaxDelta)
		{
			const T delta = aTarget - aCurrent;
			if (Abs(delta) <= aMaxDelta)
			{
				return aTarget;
			}
			return aCurrent + static_cast<T>(Sign(delta)) * aMaxDelta;
		}

		template <typename T>
		inline T MoveTowardsAngleRadians(const T aCurrent, const T aTarget, const T aMaxDelta)
		{
			return aCurrent + MoveTowards(static_cast<T>(0), DeltaAngleRadians(aCurrent, aTarget), aMaxDelta);
		}

		template <typename T>
		inline T MoveTowardsAngleDegrees(const T aCurrent, const T aTarget, const T aMaxDelta)
		{
			return aCurrent + MoveTowards(static_cast<T>(0), DeltaAngleDegrees(aCurrent, aTarget), aMaxDelta);
		}
#pragma endregion

		#pragma region Interpolation and motion
		template <typename T>
		inline T Lerp(const T aStart, const T anEnd, const T aT)
		{
			return aStart + (anEnd - aStart) * aT;
		}

		template <typename T>
		inline T LerpClamped(const T aStart, const T anEnd, const T aT)
		{
			return Lerp(aStart, anEnd, Clamp01(aT));
		}

		template <typename T>
		inline T SmoothStep(const T aT)
		{
			const T t = Clamp01(aT);
			return t * t * (static_cast<T>(3) - static_cast<T>(2) * t);
		}

		template <typename T>
		inline T SmootherStep(const T aT)
		{
			const T t = Clamp01(aT);
			return t * t * t * (t * (t * static_cast<T>(6) - static_cast<T>(15)) + static_cast<T>(10));
		}

		// Exponential damping that is frame-rate independent when aDeltaTime is seconds.
		template <typename T>
		inline T Damp(const T aCurrent, const T aTarget, const T aLambda, const T aDeltaTime)
		{
			return Lerp(aCurrent, aTarget, static_cast<T>(1) - static_cast<T>(std::exp(-aLambda * aDeltaTime)));
		}

		template <typename T>
		inline T ExpDecay(const T aValue, const T aLambda, const T aDeltaTime)
		{
			return aValue * static_cast<T>(std::exp(-aLambda * aDeltaTime));
		}

		// Critically-damped spring approximation for scalar values. Velocity is in/out so
		// callers can keep momentum across frames.
		template <typename T>
		inline T SpringDamp(const T aCurrent, const T aTarget, T& aVelocity, const T aSmoothTime, const T aDeltaTime)
		{
			const T smoothTime = Max(aSmoothTime, Epsilon<T>());
			const T omega = static_cast<T>(2) / smoothTime;
			const T x = omega * aDeltaTime;
			const T exp = static_cast<T>(1) / (static_cast<T>(1) + x + static_cast<T>(0.48) * x * x + static_cast<T>(0.235) * x * x * x);
			const T change = aCurrent - aTarget;
			const T temp = (aVelocity + omega * change) * aDeltaTime;
			aVelocity = (aVelocity - omega * temp) * exp;
			return aTarget + (change + temp) * exp;
		}
		#pragma endregion
		
		#pragma region Vector helpers
		template <typename T>
		inline Vector2<T> Lerp(const Vector2<T>& aStart, const Vector2<T>& anEnd, const T aT)
		{
			return aStart + (anEnd - aStart) * aT;
		}

		template <typename T>
		inline Vector3<T> Lerp(const Vector3<T>& aStart, const Vector3<T>& anEnd, const T aT)
		{
			return aStart + (anEnd - aStart) * aT;
		}

		template <typename T>
		inline Vector4<T> Lerp(const Vector4<T>& aStart, const Vector4<T>& anEnd, const T aT)
		{
			return aStart + (anEnd - aStart) * aT;
		}

		template <typename T>
		inline Vector2<T> LerpClamped(const Vector2<T>& aStart, const Vector2<T>& anEnd, const T aT)
		{
			return Lerp(aStart, anEnd, Clamp01(aT));
		}

		template <typename T>
		inline Vector3<T> LerpClamped(const Vector3<T>& aStart, const Vector3<T>& anEnd, const T aT)
		{
			return Lerp(aStart, anEnd, Clamp01(aT));
		}

		template <typename T>
		inline Vector4<T> LerpClamped(const Vector4<T>& aStart, const Vector4<T>& anEnd, const T aT)
		{
			return Lerp(aStart, anEnd, Clamp01(aT));
		}

		template <typename T>
		inline T Distance(const Vector2<T>& aA, const Vector2<T>& aB)
		{
			return (aA - aB).Length();
		}

		template <typename T>
		inline T Distance(const Vector3<T>& aA, const Vector3<T>& aB)
		{
			return (aA - aB).Length();
		}

		template <typename T>
		inline T Distance(const Vector4<T>& aA, const Vector4<T>& aB)
		{
			return (aA - aB).Length();
		}

		template <typename T>
		inline T DistanceSqr(const Vector2<T>& aA, const Vector2<T>& aB)
		{
			return (aA - aB).LengthSqr();
		}

		template <typename T>
		inline T DistanceSqr(const Vector3<T>& aA, const Vector3<T>& aB)
		{
			return (aA - aB).LengthSqr();
		}

		template <typename T>
		inline T DistanceSqr(const Vector4<T>& aA, const Vector4<T>& aB)
		{
			return (aA - aB).LengthSqr();
		}

		template <typename T>
		inline Vector2<T> NormalizeSafe(const Vector2<T>& aVector, const Vector2<T>& aFallback = Vector2<T>::Zero, const T anEpsilon = Epsilon<T>())
		{
			const T lengthSqr = aVector.LengthSqr();
			return lengthSqr <= anEpsilon * anEpsilon ? aFallback : aVector / static_cast<T>(std::sqrt(lengthSqr));
		}

		template <typename T>
		inline Vector3<T> NormalizeSafe(const Vector3<T>& aVector, const Vector3<T>& aFallback = Vector3<T>::Zero, const T anEpsilon = Epsilon<T>())
		{
			const T lengthSqr = aVector.LengthSqr();
			return lengthSqr <= anEpsilon * anEpsilon ? aFallback : aVector / static_cast<T>(std::sqrt(lengthSqr));
		}

		template <typename T>
		inline Vector4<T> NormalizeSafe(const Vector4<T>& aVector, const Vector4<T>& aFallback = Vector4<T>::Zero, const T anEpsilon = Epsilon<T>())
		{
			const T lengthSqr = aVector.LengthSqr();
			return lengthSqr <= anEpsilon * anEpsilon ? aFallback : aVector / static_cast<T>(std::sqrt(lengthSqr));
		}

		template <typename TVector, typename T>
		inline TVector ClampMagnitude(const TVector& aVector, const T aMaxLength)
		{
			const T lengthSqr = aVector.LengthSqr();
			const T maxLengthSqr = aMaxLength * aMaxLength;
			if (lengthSqr <= maxLengthSqr || IsZero(lengthSqr))
			{
				return aVector;
			}
			return aVector * (aMaxLength / static_cast<T>(std::sqrt(lengthSqr)));
		}

		// Projects aVector onto aNormal. Near-zero normals return zero to avoid division spikes.
		template <typename TVector, typename T>
		inline TVector Project(const TVector& aVector, const TVector& aNormal, const T anEpsilon = Epsilon<T>())
		{
			const T denominator = aNormal.LengthSqr();
			if (denominator <= anEpsilon * anEpsilon)
			{
				return TVector();
			}
			return aNormal * (aVector.Dot(aNormal) / denominator);
		}

		template <typename T>
		inline Vector2<T> Project(const Vector2<T>& aVector, const Vector2<T>& aNormal)
		{
			return Project(aVector, aNormal, Epsilon<T>());
		}

		template <typename T>
		inline Vector3<T> Project(const Vector3<T>& aVector, const Vector3<T>& aNormal)
		{
			return Project(aVector, aNormal, Epsilon<T>());
		}

		template <typename T>
		inline Vector4<T> Project(const Vector4<T>& aVector, const Vector4<T>& aNormal)
		{
			return Project(aVector, aNormal, Epsilon<T>());
		}

		template <typename TVector, typename T>
		inline TVector Reject(const TVector& aVector, const TVector& aNormal, const T anEpsilon = Epsilon<T>())
		{
			return aVector - Project(aVector, aNormal, anEpsilon);
		}

		template <typename T>
		inline Vector2<T> Reject(const Vector2<T>& aVector, const Vector2<T>& aNormal)
		{
			return Reject(aVector, aNormal, Epsilon<T>());
		}

		template <typename T>
		inline Vector3<T> Reject(const Vector3<T>& aVector, const Vector3<T>& aNormal)
		{
			return Reject(aVector, aNormal, Epsilon<T>());
		}

		template <typename T>
		inline Vector4<T> Reject(const Vector4<T>& aVector, const Vector4<T>& aNormal)
		{
			return Reject(aVector, aNormal, Epsilon<T>());
		}

		template <typename TVector>
		inline TVector Reflect(const TVector& aVector, const TVector& aNormal)
		{
			return aVector - aNormal * (static_cast<decltype(aVector.x)>(2) * aVector.Dot(aNormal));
		}

		// Returns zero when total internal reflection occurs.
		template <typename TVector, typename T>
		inline TVector Refract(const TVector& aIncident, const TVector& aNormal, const T anEta)
		{
			const T cosI = Clamp(aIncident.Dot(aNormal), static_cast<T>(-1), static_cast<T>(1));
			const T k = static_cast<T>(1) - anEta * anEta * (static_cast<T>(1) - cosI * cosI);
			if (k < static_cast<T>(0))
			{
				return TVector();
			}
			return aIncident * anEta - aNormal * (anEta * cosI + static_cast<T>(std::sqrt(k)));
		}

		template <typename TVector, typename T>
		inline T AngleBetween(const TVector& aA, const TVector& aB, const T anEpsilon = Epsilon<T>())
		{
			const T lengthProduct = aA.Length() * aB.Length();
			if (lengthProduct <= anEpsilon)
			{
				return static_cast<T>(0);
			}
			return static_cast<T>(std::acos(Clamp(aA.Dot(aB) / lengthProduct, static_cast<T>(-1), static_cast<T>(1))));
		}

		template <typename T>
		inline T AngleBetween(const Vector2<T>& aA, const Vector2<T>& aB)
		{
			return AngleBetween(aA, aB, Epsilon<T>());
		}

		template <typename T>
		inline T AngleBetween(const Vector3<T>& aA, const Vector3<T>& aB)
		{
			return AngleBetween(aA, aB, Epsilon<T>());
		}

		template <typename T>
		inline T AngleBetween(const Vector4<T>& aA, const Vector4<T>& aB)
		{
			return AngleBetween(aA, aB, Epsilon<T>());
		}
		#pragma endregion
		
		#pragma region 2D helpers
		template <typename T>
		constexpr Vector2<T> PerpendicularClockwise(const Vector2<T>& aVector)
		{
			return Vector2<T>(aVector.y, -aVector.x);
		}

		template <typename T>
		constexpr Vector2<T> PerpendicularCounterClockwise(const Vector2<T>& aVector)
		{
			return Vector2<T>(-aVector.y, aVector.x);
		}

		template <typename T>
		inline Vector2<T> Rotate(const Vector2<T>& aVector, const T anAngleRadians)
		{
			const T c = static_cast<T>(std::cos(anAngleRadians));
			const T s = static_cast<T>(std::sin(anAngleRadians));
			return Vector2<T>(aVector.x * c - aVector.y * s, aVector.x * s + aVector.y * c);
		}

		template <typename T>
		inline T SignedAngle(const Vector2<T>& aFrom, const Vector2<T>& aTo)
		{
			return static_cast<T>(std::atan2(aFrom.x * aTo.y - aFrom.y * aTo.x, aFrom.Dot(aTo)));
		}

		template <typename T>
		inline Vector2<T> DirectionFromAngle(const T anAngleRadians)
		{
			return Vector2<T>(static_cast<T>(std::cos(anAngleRadians)), static_cast<T>(std::sin(anAngleRadians)));
		}

		template <typename T>
		inline T AngleFromDirection(const Vector2<T>& aDirection)
		{
			return static_cast<T>(std::atan2(aDirection.y, aDirection.x));
		}

		template <typename T>
		inline Vector2<T> ClosestPointOnLine(const Vector2<T>& aPoint, const Vector2<T>& aLinePoint, const Vector2<T>& aLineDirection, const T anEpsilon = Epsilon<T>())
		{
			return aLinePoint + Project(aPoint - aLinePoint, aLineDirection, anEpsilon);
		}

		template <typename T>
		inline Vector2<T> ClosestPointOnSegment(const Vector2<T>& aPoint, const Vector2<T>& aStart, const Vector2<T>& anEnd, const T anEpsilon = Epsilon<T>())
		{
			const Vector2<T> segment = anEnd - aStart;
			const T lengthSqr = segment.LengthSqr();
			if (lengthSqr <= anEpsilon * anEpsilon)
			{
				return aStart;
			}
			const T t = Clamp01((aPoint - aStart).Dot(segment) / lengthSqr);
			return aStart + segment * t;
		}
		#pragma endregion
		
		#pragma region 3D helpers
		template <typename T>
		inline T SignedAngleAroundAxis(const Vector3<T>& aFrom, const Vector3<T>& aTo, const Vector3<T>& anAxis)
		{
			const Vector3<T> from = NormalizeSafe(aFrom);
			const Vector3<T> to = NormalizeSafe(aTo);
			const Vector3<T> axis = NormalizeSafe(anAxis, Vector3<T>::UnitY);
			return static_cast<T>(std::atan2(axis.Dot(from.Cross(to)), from.Dot(to)));
		}

		template <typename T>
		inline Vector3<T> ProjectOnPlane(const Vector3<T>& aVector, const Vector3<T>& aPlaneNormal, const T anEpsilon = Epsilon<T>())
		{
			return Reject(aVector, aPlaneNormal, anEpsilon);
		}

		template <typename T>
		inline Vector3<T> DirectionFromYawPitch(const T aYawRadians, const T aPitchRadians)
		{
			const T cosPitch = static_cast<T>(std::cos(aPitchRadians));
			return Vector3<T>(
				static_cast<T>(std::sin(aYawRadians)) * cosPitch,
				static_cast<T>(-std::sin(aPitchRadians)),
				static_cast<T>(std::cos(aYawRadians)) * cosPitch);
		}

		template <typename T>
		inline Vector3<T> TriangleNormal(const Vector3<T>& aA, const Vector3<T>& aB, const Vector3<T>& aC, const T anEpsilon = Epsilon<T>())
		{
			return NormalizeSafe((aB - aA).Cross(aC - aA), Vector3<T>::Zero, anEpsilon);
		}

		// Returns barycentric weights (u, v, w) for aPoint relative to triangle ABC.
		template <typename T>
		inline Vector3<T> Barycentric(const Vector3<T>& aPoint, const Vector3<T>& aA, const Vector3<T>& aB, const Vector3<T>& aC, const T anEpsilon = Epsilon<T>())
		{
			const Vector3<T> v0 = aB - aA;
			const Vector3<T> v1 = aC - aA;
			const Vector3<T> v2 = aPoint - aA;
			const T d00 = v0.Dot(v0);
			const T d01 = v0.Dot(v1);
			const T d11 = v1.Dot(v1);
			const T d20 = v2.Dot(v0);
			const T d21 = v2.Dot(v1);
			const T denominator = d00 * d11 - d01 * d01;
			if (Abs(denominator) <= anEpsilon)
			{
				return Vector3<T>::Zero;
			}
			const T v = (d11 * d20 - d01 * d21) / denominator;
			const T w = (d00 * d21 - d01 * d20) / denominator;
			const T u = static_cast<T>(1) - v - w;
			return Vector3<T>(u, v, w);
		}

		template <typename T>
		inline Vector3<T> ClosestPointOnLine(const Vector3<T>& aPoint, const Vector3<T>& aLinePoint, const Vector3<T>& aLineDirection, const T anEpsilon = Epsilon<T>())
		{
			return aLinePoint + Project(aPoint - aLinePoint, aLineDirection, anEpsilon);
		}

		template <typename T>
		inline Vector3<T> ClosestPointOnSegment(const Vector3<T>& aPoint, const Vector3<T>& aStart, const Vector3<T>& anEnd, const T anEpsilon = Epsilon<T>())
		{
			const Vector3<T> segment = anEnd - aStart;
			const T lengthSqr = segment.LengthSqr();
			if (lengthSqr <= anEpsilon * anEpsilon)
			{
				return aStart;
			}
			const T t = Clamp01((aPoint - aStart).Dot(segment) / lengthSqr);
			return aStart + segment * t;
		}

#pragma endregion

		#pragma region Quaternion helpers
		template <typename T>
		inline Quaternion<T> QuaternionFromRotationMatrix(const Matrix3x3<T>& aMatrix)
		{
			const T trace = aMatrix(1, 1) + aMatrix(2, 2) + aMatrix(3, 3);
			Quaternion<T> result;

			if (trace > static_cast<T>(0))
			{
				const T s = static_cast<T>(std::sqrt(trace + static_cast<T>(1))) * static_cast<T>(2);
				result.w = static_cast<T>(0.25) * s;
				result.x = (aMatrix(2, 3) - aMatrix(3, 2)) / s;
				result.y = (aMatrix(3, 1) - aMatrix(1, 3)) / s;
				result.z = (aMatrix(1, 2) - aMatrix(2, 1)) / s;
			}
			else if (aMatrix(1, 1) > aMatrix(2, 2) && aMatrix(1, 1) > aMatrix(3, 3))
			{
				const T s = static_cast<T>(std::sqrt(static_cast<T>(1) + aMatrix(1, 1) - aMatrix(2, 2) - aMatrix(3, 3))) * static_cast<T>(2);
				result.w = (aMatrix(2, 3) - aMatrix(3, 2)) / s;
				result.x = static_cast<T>(0.25) * s;
				result.y = (aMatrix(1, 2) + aMatrix(2, 1)) / s;
				result.z = (aMatrix(1, 3) + aMatrix(3, 1)) / s;
			}
			else if (aMatrix(2, 2) > aMatrix(3, 3))
			{
				const T s = static_cast<T>(std::sqrt(static_cast<T>(1) + aMatrix(2, 2) - aMatrix(1, 1) - aMatrix(3, 3))) * static_cast<T>(2);
				result.w = (aMatrix(3, 1) - aMatrix(1, 3)) / s;
				result.x = (aMatrix(1, 2) + aMatrix(2, 1)) / s;
				result.y = static_cast<T>(0.25) * s;
				result.z = (aMatrix(2, 3) + aMatrix(3, 2)) / s;
			}
			else
			{
				const T s = static_cast<T>(std::sqrt(static_cast<T>(1) + aMatrix(3, 3) - aMatrix(1, 1) - aMatrix(2, 2))) * static_cast<T>(2);
				result.w = (aMatrix(1, 2) - aMatrix(2, 1)) / s;
				result.x = (aMatrix(1, 3) + aMatrix(3, 1)) / s;
				result.y = (aMatrix(2, 3) + aMatrix(3, 2)) / s;
				result.z = static_cast<T>(0.25) * s;
			}

			result.Normalize();
			return result;
		}

		template <typename T>
		inline Quaternion<T> NLerp(const Quaternion<T>& aStart, const Quaternion<T>& anEnd, const T aT)
		{
			Quaternion<T> end = anEnd;
			if (aStart.Dot(end) < static_cast<T>(0))
			{
				end = Quaternion<T>(-end.w, -end.x, -end.y, -end.z);
			}

			Quaternion<T> result(
				Lerp(aStart.w, end.w, aT),
				Lerp(aStart.x, end.x, aT),
				Lerp(aStart.y, end.y, aT),
				Lerp(aStart.z, end.z, aT));
			result.Normalize();
			return result;
		}

		template <typename T>
		inline Quaternion<T> SLerp(const Quaternion<T>& aStart, const Quaternion<T>& anEnd, const T aT)
		{
			Quaternion<T> end = anEnd;
			T dot = aStart.Dot(end);
			if (dot < static_cast<T>(0))
			{
				end = Quaternion<T>(-end.w, -end.x, -end.y, -end.z);
				dot = -dot;
			}

			if (dot > static_cast<T>(0.9995))
			{
				return NLerp(aStart, end, aT);
			}

			const T theta0 = static_cast<T>(std::acos(Clamp(dot, static_cast<T>(-1), static_cast<T>(1))));
			const T theta = theta0 * aT;
			const T sinTheta = static_cast<T>(std::sin(theta));
			const T sinTheta0 = static_cast<T>(std::sin(theta0));

			const T s0 = static_cast<T>(std::cos(theta)) - dot * sinTheta / sinTheta0;
			const T s1 = sinTheta / sinTheta0;
			Quaternion<T> result(
				aStart.w * s0 + end.w * s1,
				aStart.x * s0 + end.x * s1,
				aStart.y * s0 + end.y * s1,
				aStart.z * s0 + end.z * s1);
			result.Normalize();
			return result;
		}

		template <typename T>
		inline Vector3<T> RotateVector(const Quaternion<T>& aRotation, const Vector3<T>& aVector)
		{
			return aVector * aRotation.ToMatrix3x3();
		}

		template <typename T>
		inline Quaternion<T> FromToRotation(const Vector3<T>& aFrom, const Vector3<T>& aTo, const T anEpsilon = Epsilon<T>())
		{
			const Vector3<T> from = NormalizeSafe(aFrom, Vector3<T>::UnitZ, anEpsilon);
			const Vector3<T> to = NormalizeSafe(aTo, Vector3<T>::UnitZ, anEpsilon);
			const T dot = Clamp(from.Dot(to), static_cast<T>(-1), static_cast<T>(1));

			if (dot >= static_cast<T>(1) - anEpsilon)
			{
				return Quaternion<T>();
			}

			if (dot <= static_cast<T>(-1) + anEpsilon)
			{
				Vector3<T> axis = Vector3<T>::UnitX.Cross(from);
				if (axis.LengthSqr() <= anEpsilon * anEpsilon)
				{
					axis = Vector3<T>::UnitY.Cross(from);
				}
				return Quaternion<T>::CreateFromAxisAngle(axis, Pi<T>());
			}

			const Vector3<T> axis = from.Cross(to);
			Quaternion<T> result(static_cast<T>(1) + dot, axis.x, axis.y, axis.z);
			result.Normalize();
			return result;
		}

		template <typename T>
		inline Quaternion<T> LookRotation(const Vector3<T>& aForward, const Vector3<T>& anUp = Vector3<T>::UnitY, const T anEpsilon = Epsilon<T>())
		{
			const Vector3<T> forward = NormalizeSafe(aForward, Vector3<T>::UnitZ, anEpsilon);
			Vector3<T> right = NormalizeSafe(anUp.Cross(forward), Vector3<T>::UnitX, anEpsilon);
			Vector3<T> up = NormalizeSafe(forward.Cross(right), Vector3<T>::UnitY, anEpsilon);

			Matrix3x3<T> rotation;
			rotation(1, 1) = right.x;
			rotation(1, 2) = right.y;
			rotation(1, 3) = right.z;
			rotation(2, 1) = up.x;
			rotation(2, 2) = up.y;
			rotation(2, 3) = up.z;
			rotation(3, 1) = forward.x;
			rotation(3, 2) = forward.y;
			rotation(3, 3) = forward.z;
			return QuaternionFromRotationMatrix(rotation);
		}

		// Extracts yaw(Y), pitch(X), roll(Z) in radians using the same convention as Transform.
		template <typename T>
		inline Vector3<T> YawPitchRollFromQuaternion(const Quaternion<T>& aRotation)
		{
			const Matrix3x3<T> m = aRotation.ToMatrix3x3();
			const T pitch = static_cast<T>(-std::asin(Clamp(m(3, 2), static_cast<T>(-1), static_cast<T>(1))));
			const T cosPitch = static_cast<T>(std::cos(pitch));

			if (Abs(cosPitch) <= Epsilon<T>())
			{
				return Vector3<T>(
					static_cast<T>(std::atan2(m(1, 3), m(1, 1))),
					pitch,
					static_cast<T>(0));
			}

			return Vector3<T>(
				static_cast<T>(std::atan2(m(3, 1), m(3, 3))),
				pitch,
				static_cast<T>(std::atan2(m(1, 2), m(2, 2))));
		}
#pragma endregion

		#pragma region Matrix and transform helpers
		template <typename T>
		inline Matrix4x4<T> CreateTranslation(const Vector3<T>& aTranslation)
		{
			Matrix4x4<T> result;
			result(4, 1) = aTranslation.x;
			result(4, 2) = aTranslation.y;
			result(4, 3) = aTranslation.z;
			return result;
		}

		template <typename T>
		inline Matrix4x4<T> CreateScale(const Vector3<T>& aScale)
		{
			Matrix4x4<T> result;
			result(1, 1) = aScale.x;
			result(2, 2) = aScale.y;
			result(3, 3) = aScale.z;
			return result;
		}

		// Creates a row-vector SRT matrix: localPoint * Scale * Rotation * Translation.
		template <typename T>
		inline Matrix4x4<T> CreateSRT(const Vector3<T>& aScale, const Quaternion<T>& aRotation, const Vector3<T>& aTranslation)
		{
			return CreateScale(aScale) * aRotation.ToMatrix4x4() * CreateTranslation(aTranslation);
		}

		template <typename T>
		inline Matrix4x4<T> CreatePerspectiveFovLH(const T aFieldOfViewRadians, const T anAspectRatio, const T aNearPlane, const T aFarPlane)
		{
			Matrix4x4<T> projection;
			const T f = static_cast<T>(1) / static_cast<T>(std::tan(aFieldOfViewRadians * static_cast<T>(0.5)));
			projection(1, 1) = f / anAspectRatio;
			projection(2, 2) = f;
			projection(3, 3) = aFarPlane / (aFarPlane - aNearPlane);
			projection(3, 4) = static_cast<T>(1);
			projection(4, 3) = (-aNearPlane * aFarPlane) / (aFarPlane - aNearPlane);
			projection(4, 4) = static_cast<T>(0);
			return projection;
		}

		template <typename T>
		inline Matrix4x4<T> CreateOrthographicLH(const T aLeft, const T aRight, const T aBottom, const T aTop, const T aNearPlane, const T aFarPlane)
		{
			Matrix4x4<T> projection;
			const T rightMinusLeft = aRight - aLeft;
			const T topMinusBottom = aTop - aBottom;
			const T farMinusNear = aFarPlane - aNearPlane;
			projection(1, 1) = static_cast<T>(2) / rightMinusLeft;
			projection(2, 2) = static_cast<T>(2) / topMinusBottom;
			projection(3, 3) = static_cast<T>(1) / farMinusNear;
			projection(4, 1) = -(aRight + aLeft) / rightMinusLeft;
			projection(4, 2) = -(aTop + aBottom) / topMinusBottom;
			projection(4, 3) = -aNearPlane / farMinusNear;
			return projection;
		}

		template <typename T>
		inline Matrix4x4<T> CreateOrthographicLH(const T aWidth, const T aHeight, const T aNearPlane, const T aFarPlane)
		{
			return CreateOrthographicLH(
				-aWidth * static_cast<T>(0.5),
				aWidth * static_cast<T>(0.5),
				-aHeight * static_cast<T>(0.5),
				aHeight * static_cast<T>(0.5),
				aNearPlane,
				aFarPlane);
		}

		template <typename T>
		inline Matrix4x4<T> CreateLookAtLH(const Vector3<T>& anEye, const Vector3<T>& aTarget, const Vector3<T>& anUp = Vector3<T>::UnitY, const T anEpsilon = Epsilon<T>())
		{
			const Vector3<T> forward = NormalizeSafe(aTarget - anEye, Vector3<T>::UnitZ, anEpsilon);
			const Vector3<T> right = NormalizeSafe(anUp.Cross(forward), Vector3<T>::UnitX, anEpsilon);
			const Vector3<T> up = forward.Cross(right);

			Matrix4x4<T> view;
			view(1, 1) = right.x;
			view(2, 1) = right.y;
			view(3, 1) = right.z;
			view(1, 2) = up.x;
			view(2, 2) = up.y;
			view(3, 2) = up.z;
			view(1, 3) = forward.x;
			view(2, 3) = forward.y;
			view(3, 3) = forward.z;
			view(4, 1) = -anEye.Dot(right);
			view(4, 2) = -anEye.Dot(up);
			view(4, 3) = -anEye.Dot(forward);
			return view;
		}

		template <typename T>
		inline Vector3<T> TransformPoint(const Vector3<T>& aPoint, const Matrix4x4<T>& aMatrix)
		{
			const Vector4<T> transformed = Vector4<T>(aPoint.x, aPoint.y, aPoint.z, static_cast<T>(1)) * aMatrix;
			if (IsZero(transformed.w))
			{
				return Vector3<T>(transformed.x, transformed.y, transformed.z);
			}
			return Vector3<T>(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
		}

		template <typename T>
		inline Vector3<T> TransformVector(const Vector3<T>& aVector, const Matrix4x4<T>& aMatrix)
		{
			const Vector4<T> transformed = Vector4<T>(aVector.x, aVector.y, aVector.z, static_cast<T>(0)) * aMatrix;
			return Vector3<T>(transformed.x, transformed.y, transformed.z);
		}

		template <typename T>
		inline Vector3<T> TransformDirection(const Vector3<T>& aDirection, const Matrix4x4<T>& aMatrix)
		{
			return NormalizeSafe(TransformVector(aDirection, aMatrix));
		}

		// Decomposes matrices made by CreateSRT. Shear is not represented by the outputs.
		template <typename T>
		inline bool DecomposeSRT(const Matrix4x4<T>& aMatrix, Vector3<T>& outScale, Quaternion<T>& outRotation, Vector3<T>& outTranslation, const T anEpsilon = Epsilon<T>())
		{
			outTranslation = Vector3<T>(aMatrix(4, 1), aMatrix(4, 2), aMatrix(4, 3));
			Vector3<T> right = aMatrix.GetRight();
			Vector3<T> up = aMatrix.GetUp();
			Vector3<T> forward = aMatrix.GetForward();

			outScale = Vector3<T>(right.Length(), up.Length(), forward.Length());
			if (outScale.x <= anEpsilon || outScale.y <= anEpsilon || outScale.z <= anEpsilon)
			{
				outRotation = Quaternion<T>();
				return false;
			}

			right /= outScale.x;
			up /= outScale.y;
			forward /= outScale.z;

			Matrix3x3<T> rotation;
			rotation(1, 1) = right.x;
			rotation(1, 2) = right.y;
			rotation(1, 3) = right.z;
			rotation(2, 1) = up.x;
			rotation(2, 2) = up.y;
			rotation(2, 3) = up.z;
			rotation(3, 1) = forward.x;
			rotation(3, 2) = forward.y;
			rotation(3, 3) = forward.z;
			outRotation = QuaternionFromRotationMatrix(rotation);
			return true;
		}
#pragma endregion

		#pragma region Coordinate conversion
		template <typename T>
		inline Vector2<T> CartesianToPolar(const Vector2<T>& aPosition)
		{
			return Vector2<T>(aPosition.Length(), static_cast<T>(std::atan2(aPosition.y, aPosition.x)));
		}

		template <typename T>
		inline Vector2<T> PolarToCartesian(const T aRadius, const T anAngleRadians)
		{
			return DirectionFromAngle(anAngleRadians) * aRadius;
		}

		// Spherical vector layout is (radius, yaw, pitch), using the same +Z-forward yaw
		// and pitch convention as DirectionFromYawPitch.
		template <typename T>
		inline Vector3<T> CartesianToSpherical(const Vector3<T>& aPosition, const T anEpsilon = Epsilon<T>())
		{
			const T radius = aPosition.Length();
			if (radius <= anEpsilon)
			{
				return Vector3<T>::Zero;
			}
			const T yaw = static_cast<T>(std::atan2(aPosition.x, aPosition.z));
			const T pitch = static_cast<T>(-std::asin(Clamp(aPosition.y / radius, static_cast<T>(-1), static_cast<T>(1))));
			return Vector3<T>(radius, yaw, pitch);
		}

		template <typename T>
		inline Vector3<T> SphericalToCartesian(const T aRadius, const T aYawRadians, const T aPitchRadians)
		{
			return DirectionFromYawPitch(aYawRadians, aPitchRadians) * aRadius;
		}

		template <typename T>
		inline Vector2<T> ScreenToNDC(const Vector2<T>& aScreenPosition, const Vector2<T>& aScreenSize)
		{
			return Vector2<T>(
				(aScreenPosition.x / aScreenSize.x) * static_cast<T>(2) - static_cast<T>(1),
				static_cast<T>(1) - (aScreenPosition.y / aScreenSize.y) * static_cast<T>(2));
		}

		template <typename T>
		inline Vector2<T> NDCToScreen(const Vector2<T>& anNDCPosition, const Vector2<T>& aScreenSize)
		{
			return Vector2<T>(
				(anNDCPosition.x + static_cast<T>(1)) * static_cast<T>(0.5) * aScreenSize.x,
				(static_cast<T>(1) - anNDCPosition.y) * static_cast<T>(0.5) * aScreenSize.y);
		}
#pragma endregion

		#pragma region Easing
		template <typename T>
		inline T EaseInSine(const T aT)
		{
			const T t = Clamp01(aT);
			return static_cast<T>(1) - static_cast<T>(std::cos((t * Pi<T>()) * static_cast<T>(0.5)));
		}

		template <typename T>
		inline T EaseOutSine(const T aT)
		{
			const T t = Clamp01(aT);
			return static_cast<T>(std::sin((t * Pi<T>()) * static_cast<T>(0.5)));
		}

		template <typename T>
		inline T EaseInOutSine(const T aT)
		{
			const T t = Clamp01(aT);
			return -(static_cast<T>(std::cos(Pi<T>() * t)) - static_cast<T>(1)) * static_cast<T>(0.5);
		}

		template <typename T>
		inline T EaseInQuad(const T aT)
		{
			const T t = Clamp01(aT);
			return t * t;
		}

		template <typename T>
		inline T EaseOutQuad(const T aT)
		{
			const T t = Clamp01(aT);
			return static_cast<T>(1) - (static_cast<T>(1) - t) * (static_cast<T>(1) - t);
		}

		template <typename T>
		inline T EaseInOutQuad(const T aT)
		{
			const T t = Clamp01(aT);
			return t < static_cast<T>(0.5)
				? static_cast<T>(2) * t * t
				: static_cast<T>(1) - Square(static_cast<T>(-2) * t + static_cast<T>(2)) * static_cast<T>(0.5);
		}

		template <typename T>
		inline T EaseInCubic(const T aT)
		{
			const T t = Clamp01(aT);
			return t * t * t;
		}

		template <typename T>
		inline T EaseOutCubic(const T aT)
		{
			const T t = Clamp01(aT);
			return static_cast<T>(1) - Cube(static_cast<T>(1) - t);
		}

		template <typename T>
		inline T EaseInOutCubic(const T aT)
		{
			const T t = Clamp01(aT);
			return t < static_cast<T>(0.5)
				? static_cast<T>(4) * t * t * t
				: static_cast<T>(1) - Cube(static_cast<T>(-2) * t + static_cast<T>(2)) * static_cast<T>(0.5);
		}

		template <typename T>
		inline T EaseInBack(const T aT)
		{
			const T t = Clamp01(aT);
			const T c1 = static_cast<T>(1.70158);
			const T c3 = c1 + static_cast<T>(1);
			return c3 * t * t * t - c1 * t * t;
		}

		template <typename T>
		inline T EaseOutBack(const T aT)
		{
			const T t = Clamp01(aT) - static_cast<T>(1);
			const T c1 = static_cast<T>(1.70158);
			const T c3 = c1 + static_cast<T>(1);
			return static_cast<T>(1) + c3 * t * t * t + c1 * t * t;
		}

		template <typename T>
		inline T EaseInOutBack(const T aT)
		{
			const T t = Clamp01(aT);
			const T c1 = static_cast<T>(1.70158);
			const T c2 = c1 * static_cast<T>(1.525);
			return t < static_cast<T>(0.5)
				? (Square(static_cast<T>(2) * t) * ((c2 + static_cast<T>(1)) * static_cast<T>(2) * t - c2)) * static_cast<T>(0.5)
				: (Square(static_cast<T>(2) * t - static_cast<T>(2)) * ((c2 + static_cast<T>(1)) * (t * static_cast<T>(2) - static_cast<T>(2)) + c2) + static_cast<T>(2)) * static_cast<T>(0.5);
		}
#pragma endregion
	}
}
