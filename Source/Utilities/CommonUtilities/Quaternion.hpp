#pragma once
#include <cmath>
#include <assert.h>
#include <type_traits>
#include "Vector3.hpp"
#include "Matrix3x3.hpp"
#include <algorithm>

namespace CommonUtilities
{
	template <typename T>
	class Quaternion
	{
	public:

		T x;
		T y;
		T z;
		T w;

		Quaternion();
		Quaternion(const T& aX, const T& aY, const T& aZ, const T& aW);

		Quaternion(const Quaternion<T>& aQuaternion) = default;
		Quaternion<T>& operator=(const Quaternion<T>& aQuaternion) = default;
		~Quaternion() = default;

		Quaternion<T> operator-() const;

		// Creation
		static Quaternion<T> CreateIdentity();
		static Quaternion<T> CreateFromAxisAngle(const Vector3<T>& aAxis, const T& aAngleInRadians);
		//Takes radians
		static Quaternion<T> CreateFromEulerAngles(const Vector3<T>& someEulerAngles);
		static Quaternion<T> CreateFromRotationMatrix(const Matrix3x3<T>& aRotationMatrix);

		//Inspect
		T LengthSqr() const;
		T Length() const;
		T Dot(const Quaternion<T>& aQuaternion) const;

		//Modify
		void Normalize();
		Quaternion<T> GetNormalized() const;

		//Math
		Quaternion<T> GetConjugate() const;
		Quaternion<T> GetInverse() const;

		//Conversions
		Matrix3x3<T> GetRotationMatrix() const;
		Vector3<T> GetEulerAngles() const;

		//Interpolation
		static Quaternion<T> Slerp(const Quaternion<T>& aQ0, const Quaternion<T>& aQ1, T aPercentage);

	};

	//Modify
	template <typename T>
	Quaternion<T> operator+(const Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1);

	template <typename T>
	Quaternion<T> operator-(const Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1);

	//Math
	template <typename T>
	Quaternion<T> operator*(const Quaternion<T>& aQ0, const Quaternion<T>& aQ1);

	template <typename T>
	Vector3<T> operator*(const Quaternion<T>& aQuaternion, const Vector3<T>& aVector);

	//Modfify
	template <typename T>
	Quaternion<T> operator*(const Quaternion<T>& aQuaternion, const T& aScalar);

	template <typename T>
	Quaternion<T> operator*(const T& aScalar, const Quaternion<T>& aQuaternion);

	template <typename T>
	Quaternion<T> operator/(const Quaternion<T>& aQuaternion, const T& aScalar);

	template <typename T>
	void operator+=(Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1);

	template <typename T>
	void operator-=(Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1);

	template <typename T>
	void operator*=(Quaternion<T>& aQuaternion, const T& aScalar);

	template <typename T>
	void operator/=(Quaternion<T>& aQuaternion, const T& aScalar);

	//Checks
	template<typename T>
	bool operator==(const Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1);

	template<typename T>
	bool operator!=(const Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1);

	template<typename T>
	inline Quaternion<T>::Quaternion() : x(0), y(0), z(0), w(1)
	{
	}

	template<typename T>
	inline Quaternion<T>::Quaternion(const T& aX, const T& aY, const T& aZ, const T& aW)
	{
		x = aX;
		y = aY;
		z = aZ;
		w = aW;
	}
	template<typename T>
	inline Quaternion<T> Quaternion<T>::operator-() const
	{
		return Quaternion<T>(-x, -y, -z, -w);
	}
	template<typename T>
	inline Quaternion<T> Quaternion<T>::CreateIdentity()
	{
		return Quaternion<T>(0, 0, 0, 1);
	}

	template<typename T>
	inline Quaternion<T> Quaternion<T>::CreateFromAxisAngle(const Vector3<T>& aAxis, const T& aAngleInRadians)
	{
		Vector3<T> normalizedAxis = aAxis.GetNormalized();
		T halfAngle = aAngleInRadians / 2;

		T sinHalf = std::sin(halfAngle);
		T cosHalf = std::cos(halfAngle);

		Quaternion<T> result;

		result.x = normalizedAxis.x * sinHalf;
		result.y = normalizedAxis.y * sinHalf;
		result.z = normalizedAxis.z * sinHalf;
		result.w = cosHalf;

		return result;
	}

	template<typename T>
	inline Quaternion<T> Quaternion<T>::CreateFromEulerAngles(const Vector3<T>& someEulerAngles)
	{
		//Old version

		//T pitch = someEulerAngles.x;
		//T yaw = someEulerAngles.y;
		//T roll = someEulerAngles.z;

		//Quaternion<T> qX = CreateFromAxisAngle({ 1, 0, 0 }, pitch);
		//Quaternion<T> qY = CreateFromAxisAngle({ 0, 1, 0 }, yaw);
		//Quaternion<T> qZ = CreateFromAxisAngle({ 0, 0, 1 }, roll);

		//return qY * qX * qZ;

		//Optimized

		T halfPitch = someEulerAngles.x * static_cast<T>(0.5);
		T halfYaw = someEulerAngles.y * static_cast<T>(0.5);
		T halfRoll = someEulerAngles.z * static_cast<T>(0.5);

		T sinPitch = std::sin(halfPitch);
		T cosPitch = std::cos(halfPitch);

		T sinYaw = std::sin(halfYaw);
		T cosYaw = std::cos(halfYaw);

		T sinRoll = std::sin(halfRoll);
		T cosRoll = std::cos(halfRoll);

		return Quaternion<T>((sinPitch * cosYaw * cosRoll) - (cosPitch * sinYaw * sinRoll),
							 (cosPitch * sinYaw * cosRoll) + (sinPitch * cosYaw * sinRoll),
							 (cosPitch * cosYaw * sinRoll) + (sinPitch * sinYaw * cosRoll),
							 (cosPitch * cosYaw * cosRoll) - (sinPitch * sinYaw * sinRoll));
	}

	template<typename T>
	inline Quaternion<T> Quaternion<T>::CreateFromRotationMatrix(const Matrix3x3<T>& aRotationMatrix)
	{
		T trace = aRotationMatrix(1, 1) + aRotationMatrix(2, 2) + aRotationMatrix(3, 3);

		T x = static_cast<T>(0);
		T y = static_cast<T>(0);
		T z = static_cast<T>(0);
		T w = static_cast<T>(0);

		if (trace > 0)
		{
			w = std::sqrt(trace + static_cast<T>(1)) / static_cast<T>(2);

			T denominator = static_cast<T>(4) * w;

			x = (aRotationMatrix(2, 3) - aRotationMatrix(3, 2)) / denominator;
			y = (aRotationMatrix(3, 1) - aRotationMatrix(1, 3)) / denominator;
			z = (aRotationMatrix(1, 2) - aRotationMatrix(2, 1)) / denominator;
		}
		else
		{
			T xScore = 1 + aRotationMatrix(1, 1) - aRotationMatrix(2, 2) - aRotationMatrix(3, 3);
			T yScore = 1 - aRotationMatrix(1, 1) + aRotationMatrix(2, 2) - aRotationMatrix(3, 3);
			T zScore = 1 - aRotationMatrix(1, 1) - aRotationMatrix(2, 2) + aRotationMatrix(3, 3);

			if (xScore >= yScore && xScore >= zScore)
			{
				x = std::sqrt(xScore) / static_cast<T>(2);

				T denominator = static_cast<T>(4) * x;

				y = (aRotationMatrix(1, 2) + aRotationMatrix(2, 1)) / denominator;
				z = (aRotationMatrix(1, 3) + aRotationMatrix(3, 1)) / denominator;
				w = (aRotationMatrix(3, 2) - aRotationMatrix(2, 3)) / denominator;
			}
			else if (yScore >= zScore)
			{
				y = std::sqrt(yScore) / static_cast<T>(2);

				T denominator = static_cast<T>(4) * y;

				x = (aRotationMatrix(1, 2) + aRotationMatrix(2, 1)) / denominator;
				z = (aRotationMatrix(2, 3) + aRotationMatrix(3, 2)) / denominator;
				w = (aRotationMatrix(3, 1) - aRotationMatrix(1, 3)) / denominator;

			}
			else
			{
				z = std::sqrt(zScore) / static_cast<T>(2);

				T denominator = static_cast<T>(4) * z;

				x = (aRotationMatrix(1, 3) + aRotationMatrix(3, 1)) / denominator;
				y = (aRotationMatrix(2, 3) + aRotationMatrix(3, 2)) / denominator;
				w = (aRotationMatrix(2, 1) - aRotationMatrix(1, 2)) / denominator;

			}
		}

		return Quaternion<T>(x, y, z, w);
	}

	template<typename T>
	inline T Quaternion<T>::LengthSqr() const
	{
		return (x * x) + (y * y) + (z * z) + (w * w);
	}

	template<typename T>
	inline T Quaternion<T>::Length() const
	{
		return static_cast<T>(std::sqrt(LengthSqr()));
	}

	template<typename T>
	inline T Quaternion<T>::Dot(const Quaternion<T>& aQuaternion) const
	{
		return x * aQuaternion.x + y * aQuaternion.y + z * aQuaternion.z + w * aQuaternion.w;
	}

	template<typename T>
	inline void Quaternion<T>::Normalize()
	{
		T length = Length();
		if (length == 0)
		{
			return;
		}

		x /= length;
		y /= length;
		z /= length;
		w /= length;
	}

	template<typename T>
	inline Quaternion<T> Quaternion<T>::GetNormalized() const
	{
		Quaternion<T> copy(x, y, z, w);
		copy.Normalize();
		return copy;
	}

	template<typename T>
	inline Quaternion<T> Quaternion<T>::GetConjugate() const
	{
		return Quaternion<T>(-x, -y, -z, w);
	}

	template<typename T>
	inline Quaternion<T> Quaternion<T>::GetInverse() const
	{
		T lengthSqr = LengthSqr();

		assert(lengthSqr != 0);

		return GetConjugate() / lengthSqr;
	}

	template<typename T>
	inline Matrix3x3<T> Quaternion<T>::GetRotationMatrix() const
	{
		T x2 = x * x;
		T y2 = y * y;
		T z2 = z * z;

		T xy = x * y;
		T xz = x * z;
		T yz = y * z;

		T xw = x * w;
		T yw = y * w;
		T zw = z * w;

		Matrix3x3<T> result;

		result(1, 1) = 1 - 2 * (y2 + z2);
		result(1, 2) = 2 * (xy + zw);
		result(1, 3) = 2 * (xz - yw);

		result(2, 1) = 2 * (xy - zw);
		result(2, 2) = 1 - 2 * (x2 + z2);
		result(2, 3) = 2 * (yz + xw);

		result(3, 1) = 2 * (xz + yw);
		result(3, 2) = 2 * (yz - xw);
		result(3, 3) = 1 - 2 * (x2 + y2);

		return result;
	}

	template<typename T>
	inline Vector3<T> Quaternion<T>::GetEulerAngles() const
	{
		T pitchValue = static_cast<T>(2) * (x * w - y * z);
		pitchValue = std::clamp(pitchValue, static_cast<T>(-1), static_cast<T>(1));

		T pitch = std::asin(pitchValue);

		T yaw = std::atan2(static_cast<T>(2) * (x * z + y * w), static_cast<T>(1) - static_cast<T>(2) * (x * x + y * y));

		T roll = std::atan2(static_cast<T>(2) * (x * y + z * w), static_cast<T>(1) - static_cast<T>(2) * (x * x + z * z));


		return Vector3<T>(pitch, yaw, roll);
	}

	template<typename T>
	inline Quaternion<T> Quaternion<T>::Slerp(const Quaternion<T>& aQA, const Quaternion<T>& aQB, T aPercentage)
	{
		Quaternion<T> result;

		T dot = aQA.Dot(aQB);

		Quaternion<T> bCopy = aQB;

		// Prevent longest path
		if (dot < static_cast<T>(0))
		{
			bCopy = -bCopy;
			dot = -dot;
		}

		// Prevent acos value outside [-1, 1]
		dot = std::clamp(dot, static_cast<T>(-1), static_cast<T>(1));

		T oneMinusT = 1 - aPercentage;

		// Quaternions are close -> Normalized Lerp
		if (dot > static_cast<T>(0.9995f))
		{
			result = aQA * oneMinusT + bCopy * aPercentage;
			result.Normalize();
			return result;
		}

		T theta = std::acos(dot);
		T sinTheta = std::sin(theta);
		T weightA = std::sin(oneMinusT * theta) / sinTheta;
		T weightB = std::sin(aPercentage * theta) / sinTheta;
		result = aQA * weightA + bCopy * weightB;
		result.Normalize();

		return result;
	}

	template<typename T>
	Quaternion<T> operator+(const Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1)
	{
		return Quaternion<T>(aQuaternion0.x + aQuaternion1.x, aQuaternion0.y + aQuaternion1.y, aQuaternion0.z + aQuaternion1.z, aQuaternion0.w + aQuaternion1.w);
	}

	template<typename T>
	Quaternion<T> operator-(const Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1)
	{
		return Quaternion<T>(aQuaternion0.x - aQuaternion1.x, aQuaternion0.y - aQuaternion1.y, aQuaternion0.z - aQuaternion1.z, aQuaternion0.w - aQuaternion1.w);
	}

	template<typename T>
	Quaternion<T> operator*(const Quaternion<T>& aQ0, const Quaternion<T>& aQ1)
	{

		Vector3<T> v0 = { aQ0.x, aQ0.y, aQ0.z };
		Vector3<T> v1 = { aQ1.x, aQ1.y, aQ1.z };

		Vector3<T> vectorPart = (v0 * aQ1.w) + (v1 * aQ0.w) + v0.Cross(v1);

		T scalarPart = (aQ0.w * aQ1.w) - v0.Dot(v1);

		return Quaternion<T>(vectorPart.x, vectorPart.y, vectorPart.z, scalarPart);
	}

	//Rotates a vector - Quaternion needs to be normalized.
	template<typename T>
	Vector3<T> operator*(const Quaternion<T>& aQuaternion, const Vector3<T>& aVector)
	{
		Vector3<T> qVector = { aQuaternion.x, aQuaternion.y, aQuaternion.z };

		Vector3<T> vectorA = qVector.Cross(aVector);
		Vector3<T> vectorB = qVector.Cross(vectorA);

		vectorA *= static_cast<T>(2) * aQuaternion.w;
		vectorB *= static_cast<T>(2);

		return aVector + vectorA + vectorB;
	}

	template<typename T>
	Quaternion<T> operator*(const Quaternion<T>& aQuaternion, const T& aScalar)
	{
		return Quaternion<T>(aQuaternion.x * aScalar, aQuaternion.y * aScalar, aQuaternion.z * aScalar, aQuaternion.w * aScalar);
	}

	template<typename T>
	Quaternion<T> operator*(const T& aScalar, const Quaternion<T>& aQuaternion)
	{
		return Quaternion<T>(aQuaternion.x * aScalar, aQuaternion.y * aScalar, aQuaternion.z * aScalar, aQuaternion.w * aScalar);
	}

	template<typename T>
	Quaternion<T> operator/(const Quaternion<T>& aQuaternion, const T& aScalar)
	{
		Quaternion<T> QuaternionCopy(aQuaternion);
		QuaternionCopy /= aScalar;
		return QuaternionCopy;
	}

	template<typename T>
	void operator+=(Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1)
	{
		aQuaternion0.x += aQuaternion1.x;
		aQuaternion0.y += aQuaternion1.y;
		aQuaternion0.z += aQuaternion1.z;
		aQuaternion0.w += aQuaternion1.w;
	}

	template<typename T>
	void operator-=(Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1)
	{
		aQuaternion0.x -= aQuaternion1.x;
		aQuaternion0.y -= aQuaternion1.y;
		aQuaternion0.z -= aQuaternion1.z;
		aQuaternion0.w -= aQuaternion1.w;
	}

	template<typename T>
	void operator*=(Quaternion<T>& aQuaternion, const T& aScalar)
	{
		aQuaternion.x *= aScalar;
		aQuaternion.y *= aScalar;
		aQuaternion.z *= aScalar;
		aQuaternion.w *= aScalar;
	}

	template<typename T>
	void operator/=(Quaternion<T>& aQuaternion, const T& aScalar)
	{
		assert(aScalar != 0);

		aQuaternion.x /= aScalar;
		aQuaternion.y /= aScalar;
		aQuaternion.z /= aScalar;
		aQuaternion.w /= aScalar;
	}

	template<typename T>
	bool operator==(const Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1)
	{
		T epsilon = static_cast<T>(0.00001f);
		if (std::is_floating_point_v<T>)
		{
			return (std::abs(aQuaternion0.x - aQuaternion1.x) < epsilon) &&
				(std::abs(aQuaternion0.y - aQuaternion1.y) < epsilon) &&
				(std::abs(aQuaternion0.z - aQuaternion1.z) < epsilon) &&
				(std::abs(aQuaternion0.w - aQuaternion1.w) < epsilon);
		}
		else if (std::is_integral_v<T>)
		{
			return (aQuaternion0.x == aQuaternion1.x) &&
				(aQuaternion0.y == aQuaternion1.y) &&
				(aQuaternion0.z == aQuaternion1.z) &&
				(aQuaternion0.w == aQuaternion1.w);
		}

		return false;
	}

	template<typename T>
	bool operator!=(const Quaternion<T>& aQuaternion0, const Quaternion<T>& aQuaternion1)
	{
		return !(aQuaternion0 == aQuaternion1);
	}

}
