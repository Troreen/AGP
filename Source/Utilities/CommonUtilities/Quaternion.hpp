#pragma once

#include <cmath>

#include "Matrix3x3.hpp"
#include "Matrix4x4.hpp"
#include "Vector3.hpp"

namespace CommonUtilities
{
	template <typename T>
	class Quaternion
	{
	public:
		T w;
		T x;
		T y;
		T z;

		Quaternion();
		Quaternion(const T& aW, const T& aX, const T& aY, const T& aZ);

		static Quaternion CreateFromAxisAngle(const Vector3<T>& aAxis, const T aAngleInRadians);
		// Yaw (Y), Pitch (X), Roll (Z) in radians. Row-vector convention expects
		// multiplication order: R = Ryaw * Rpitch * Rroll.
		static Quaternion CreateFromYawPitchRoll(const T aYawRadians, const T aPitchRadians, const T aRollRadians);

		T LengthSqr() const;
		T Length() const;
		T Dot(const Quaternion& aOther) const;

		void Normalize();
		Quaternion GetNormalized() const;
		Quaternion GetConjugate() const;

		Matrix3x3<T> ToMatrix3x3() const;
		Matrix4x4<T> ToMatrix4x4() const;

		Vector3<T> GetRight() const;
		Vector3<T> GetUp() const;
		Vector3<T> GetForward() const;
	};

	template <typename T>
	Quaternion<T> operator*(const Quaternion<T>& aQuat0, const Quaternion<T>& aQuat1);

	template <typename T>
	Quaternion<T> operator*(const Quaternion<T>& aQuat, const T& aScalar);

	template <typename T>
	Quaternion<T> operator*(const T& aScalar, const Quaternion<T>& aQuat);

	template <typename T>
	void operator*=(Quaternion<T>& aQuat0, const Quaternion<T>& aQuat1);

	// --- Implementation ---

	template <typename T>
	inline Quaternion<T>::Quaternion()
		: w(static_cast<T>(1))
		, x(static_cast<T>(0))
		, y(static_cast<T>(0))
		, z(static_cast<T>(0))
	{
	}

	template <typename T>
	inline Quaternion<T>::Quaternion(const T& aW, const T& aX, const T& aY, const T& aZ)
		: w(aW)
		, x(aX)
		, y(aY)
		, z(aZ)
	{
	}

	template <typename T>
	inline Quaternion<T> Quaternion<T>::CreateFromAxisAngle(const Vector3<T>& aAxis, const T aAngleInRadians)
	{
		Vector3<T> axis = aAxis;
		const T axisLength = axis.Length();
		if (axisLength == static_cast<T>(0))
		{
			return Quaternion();
		}
		axis /= axisLength;

		const T halfAngle = aAngleInRadians * static_cast<T>(0.5);
		const T s = static_cast<T>(std::sin(halfAngle));
		const T c = static_cast<T>(std::cos(halfAngle));

		return Quaternion(c, axis.x * s, axis.y * s, axis.z * s);
	}

	template <typename T>
	inline Quaternion<T> Quaternion<T>::CreateFromYawPitchRoll(const T aYawRadians, const T aPitchRadians, const T aRollRadians)
	{
		const Quaternion<T> qYaw = Quaternion<T>::CreateFromAxisAngle(Vector3<T>::UnitY, aYawRadians);
		const Quaternion<T> qPitch = Quaternion<T>::CreateFromAxisAngle(Vector3<T>::UnitX, aPitchRadians);
		const Quaternion<T> qRoll = Quaternion<T>::CreateFromAxisAngle(Vector3<T>::UnitZ, aRollRadians);
		// Row-vector convention: M(q1*q2) == M(q2) * M(q1)
		// Desired order: Ryaw * Rpitch * Rroll
		return qRoll * qPitch * qYaw;
	}

	template <typename T>
	inline T Quaternion<T>::LengthSqr() const
	{
		return w * w + x * x + y * y + z * z;
	}

	template <typename T>
	inline T Quaternion<T>::Length() const
	{
		return static_cast<T>(std::sqrt(LengthSqr()));
	}

	template <typename T>
	inline T Quaternion<T>::Dot(const Quaternion& aOther) const
	{
		return w * aOther.w + x * aOther.x + y * aOther.y + z * aOther.z;
	}

	template <typename T>
	inline void Quaternion<T>::Normalize()
	{
		const T len = Length();
		if (len == static_cast<T>(0))
		{
			w = static_cast<T>(1);
			x = static_cast<T>(0);
			y = static_cast<T>(0);
			z = static_cast<T>(0);
			return;
		}
		const T inv = static_cast<T>(1) / len;
		w *= inv;
		x *= inv;
		y *= inv;
		z *= inv;
	}

	template <typename T>
	inline Quaternion<T> Quaternion<T>::GetNormalized() const
	{
		Quaternion<T> result = *this;
		result.Normalize();
		return result;
	}

	template <typename T>
	inline Quaternion<T> Quaternion<T>::GetConjugate() const
	{
		return Quaternion<T>(w, -x, -y, -z);
	}

	template <typename T>
	inline Matrix3x3<T> Quaternion<T>::ToMatrix3x3() const
	{
		const T xx = x * x;
		const T yy = y * y;
		const T zz = z * z;
		const T xy = x * y;
		const T xz = x * z;
		const T yz = y * z;
		const T wx = w * x;
		const T wy = w * y;
		const T wz = w * z;

		Matrix3x3<T> result;
		result(1, 1) = static_cast<T>(1) - static_cast<T>(2) * (yy + zz);
		result(1, 2) = static_cast<T>(2) * (xy + wz);
		result(1, 3) = static_cast<T>(2) * (xz - wy);

		result(2, 1) = static_cast<T>(2) * (xy - wz);
		result(2, 2) = static_cast<T>(1) - static_cast<T>(2) * (xx + zz);
		result(2, 3) = static_cast<T>(2) * (yz + wx);

		result(3, 1) = static_cast<T>(2) * (xz + wy);
		result(3, 2) = static_cast<T>(2) * (yz - wx);
		result(3, 3) = static_cast<T>(1) - static_cast<T>(2) * (xx + yy);

		return result;
	}

	template <typename T>
	inline Matrix4x4<T> Quaternion<T>::ToMatrix4x4() const
	{
		return Matrix4x4<T>(ToMatrix3x3());
	}

	template <typename T>
	inline Vector3<T> Quaternion<T>::GetRight() const
	{
		return ToMatrix3x3().GetRight();
	}

	template <typename T>
	inline Vector3<T> Quaternion<T>::GetUp() const
	{
		return ToMatrix3x3().GetUp();
	}

	template <typename T>
	inline Vector3<T> Quaternion<T>::GetForward() const
	{
		return ToMatrix3x3().GetForward();
	}

	template <typename T>
	inline Quaternion<T> operator*(const Quaternion<T>& aQuat0, const Quaternion<T>& aQuat1)
	{
		return Quaternion<T>(
			(aQuat0.w * aQuat1.w) - (aQuat0.x * aQuat1.x) - (aQuat0.y * aQuat1.y) - (aQuat0.z * aQuat1.z),
			(aQuat0.w * aQuat1.x) + (aQuat0.x * aQuat1.w) + (aQuat0.y * aQuat1.z) - (aQuat0.z * aQuat1.y),
			(aQuat0.w * aQuat1.y) - (aQuat0.x * aQuat1.z) + (aQuat0.y * aQuat1.w) + (aQuat0.z * aQuat1.x),
			(aQuat0.w * aQuat1.z) + (aQuat0.x * aQuat1.y) - (aQuat0.y * aQuat1.x) + (aQuat0.z * aQuat1.w));
	}

	template <typename T>
	inline Quaternion<T> operator*(const Quaternion<T>& aQuat, const T& aScalar)
	{
		return Quaternion<T>(aQuat.w * aScalar, aQuat.x * aScalar, aQuat.y * aScalar, aQuat.z * aScalar);
	}

	template <typename T>
	inline Quaternion<T> operator*(const T& aScalar, const Quaternion<T>& aQuat)
	{
		return Quaternion<T>(aQuat.w * aScalar, aQuat.x * aScalar, aQuat.y * aScalar, aQuat.z * aScalar);
	}

	template <typename T>
	inline void operator*=(Quaternion<T>& aQuat0, const Quaternion<T>& aQuat1)
	{
		const T w = aQuat0.w;
		const T x = aQuat0.x;
		const T y = aQuat0.y;
		const T z = aQuat0.z;

		aQuat0.w = (w * aQuat1.w) - (x * aQuat1.x) - (y * aQuat1.y) - (z * aQuat1.z);
		aQuat0.x = (w * aQuat1.x) + (x * aQuat1.w) + (y * aQuat1.z) - (z * aQuat1.y);
		aQuat0.y = (w * aQuat1.y) - (x * aQuat1.z) + (y * aQuat1.w) + (z * aQuat1.x);
		aQuat0.z = (w * aQuat1.z) + (x * aQuat1.y) - (y * aQuat1.x) + (z * aQuat1.w);
	}
}
