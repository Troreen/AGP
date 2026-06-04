#pragma once
#include <cmath>
#include <ostream>

namespace Tga
{
	template <typename T>
	class Vector4;
}

namespace CommonUtilities
{
	template <typename T>
	class Vector2;
	template <typename T>
	class Vector3;

	template <typename T>
	class Vector4
	{
	public:
		T x;
		T y;
		T z;
		T w;

		// Common constants
		static const Vector4<T> Zero;
		static const Vector4<T> One;
		static const Vector4<T> UnitX;
		static const Vector4<T> UnitY;
		static const Vector4<T> UnitZ;
		static const Vector4<T> UnitW;

		Vector4();
		Vector4(const T& aX, const T& aY, const T& aZ, const T& aW);
		Vector4(const Vector4<T>&) = default;
		Vector4& operator=(const Vector4<T>&) = default;
		~Vector4() = default;

		template<class TargetType>
		Vector4<TargetType> ToType() const;
		Tga::Vector4<T> ToTga() const;

		Vector4<T> operator-() const;
		T LengthSqr() const;
		T Length() const;
		Vector4<T> GetNormalized() const;
		void Normalize();
		T Dot(const Vector4<T>& aVector) const;

		T Distance(const Vector4<T>& anOther) const;
		T DistanceSqr(const Vector4<T>& anOther) const;
		Vector4<T> Lerp(const Vector4<T>& anOther, T aT) const;
		Vector4<T> Reflect(const Vector4<T>& aNormal) const;

		// Dimension changing helpers
		Vector2<T> ToVector2() const;
		Vector3<T> ToVector3() const;

		bool operator==(const Vector4<T>& anOther) const;
		bool operator!=(const Vector4<T>& anOther) const;
	};

	template <typename T>
	Vector4<T> operator+(const Vector4<T>& aVector0, const Vector4<T>& aVector1);

	template <typename T>
	Vector4<T> operator-(const Vector4<T>& aVector0, const Vector4<T>& aVector1);

	template <typename T>
	Vector4<T> operator*(const Vector4<T>& aVector0, const Vector4<T>& aVector1);

	template <typename T>
	Vector4<T> operator*(const Vector4<T>& aVector, const T& aScalar);

	template <typename T>
	Vector4<T> operator*(const T& aScalar, const Vector4<T>& aVector);

	template <typename T>
	Vector4<T> operator/(const Vector4<T>& aVector, const T& aScalar);

	template <typename T>
	void operator+=(Vector4<T>& aVector0, const Vector4<T>& aVector1);

	template <typename T>
	void operator-=(Vector4<T>& aVector0, const Vector4<T>& aVector1);

	template <typename T>
	void operator*=(Vector4<T>& aVector, const T& aScalar);

	template <typename T>
	void operator/=(Vector4<T>& aVector, const T& aScalar);

	template <typename T>
	std::ostream& operator<<(std::ostream& aOut, const Vector4<T>& aVector);

	//================
	// Implementations
	//================

	template<typename T>
	const Vector4<T> Vector4<T>::Zero = Vector4<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));

	template<typename T>
	const Vector4<T> Vector4<T>::One = Vector4<T>(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1), static_cast<T>(1));

	template<typename T>
	const Vector4<T> Vector4<T>::UnitX = Vector4<T>(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));

	template<typename T>
	const Vector4<T> Vector4<T>::UnitY = Vector4<T>(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));

	template<typename T>
	const Vector4<T> Vector4<T>::UnitZ = Vector4<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));

	template<typename T>
	const Vector4<T> Vector4<T>::UnitW = Vector4<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

	template<typename T>
	inline Vector4<T>::Vector4()
		: x(Vector4<T>::Zero.x),
		y(Vector4<T>::Zero.y),
		z(Vector4<T>::Zero.z),
		w(Vector4<T>::Zero.w)
	{
	}

	template<typename T>
	inline Vector4<T>::Vector4(const T& aX, const T& aY, const T& aZ, const T& aW)
		: x(aX),
		y(aY),
		z(aZ),
		w(aW)
	{
	}

	template<typename T>
	inline Tga::Vector4<T> Vector4<T>::ToTga() const
	{
		return Tga::Vector4<T>(x, y, z, w);
	}

	template<typename T>
	inline Vector4<T> Vector4<T>::operator-() const
	{
		return Vector4<T>(-x, -y, -z, -w);
	}

	template<typename T>
	inline T Vector4<T>::LengthSqr() const
	{
		return x * x + y * y + z * z + w * w;
	}

	template<typename T>
	inline T Vector4<T>::Length() const
	{
		return static_cast<T>(std::sqrt(LengthSqr()));
	}

	template<typename T>
	inline Vector4<T> Vector4<T>::GetNormalized() const
	{
		const T zero = static_cast<T>(0);
		T len = Length();
		if (len == zero)
		{
			return Vector4<T>::Zero;
		}
		const T inv = static_cast<T>(1) / len;
		return Vector4<T>(x * inv, y * inv, z * inv, w * inv);
	}

	template<typename T>
	inline void Vector4<T>::Normalize()
	{
		const T zero = static_cast<T>(0);
		T len = Length();
		if (len == zero)
		{
			x = Vector4<T>::Zero.x;
			y = Vector4<T>::Zero.y;
			z = Vector4<T>::Zero.z;
			w = Vector4<T>::Zero.w;
			return;
		}
		const T inv = static_cast<T>(1) / len;
		x *= inv; y *= inv; z *= inv; w *= inv;
	}

	template<typename T>
	inline T Vector4<T>::Dot(const Vector4<T>& aVector) const
	{
		return x * aVector.x + y * aVector.y + z * aVector.z + w * aVector.w;
	}

	template<typename T>
	inline T Vector4<T>::Distance(const Vector4<T>& anOther) const
	{
		return (*this - anOther).Length();
	}

	template<typename T>
	inline T Vector4<T>::DistanceSqr(const Vector4<T>& anOther) const
	{
		return (*this - anOther).LengthSqr();
	}

	template<typename T>
	inline Vector4<T> Vector4<T>::Lerp(const Vector4<T>& anOther, T aT) const
	{
		return Vector4<T>(
			x + (anOther.x - x) * aT,
			y + (anOther.y - y) * aT,
			z + (anOther.z - z) * aT,
			w + (anOther.w - w) * aT);
	}

	template<typename T>
	template<class TargetType>
	inline Vector4<TargetType> Vector4<T>::ToType() const
	{
		return Vector4<TargetType>(
			static_cast<TargetType>(x),
			static_cast<TargetType>(y),
			static_cast<TargetType>(z),
			static_cast<TargetType>(w));
	}

	template<typename T>
	inline Vector4<T> Vector4<T>::Reflect(const Vector4<T>& aNormal) const
	{
		return *this - aNormal * (static_cast<T>(2) * this->Dot(aNormal));
	}

	template<typename T>
	inline Vector2<T> Vector4<T>::ToVector2() const
	{
		return Vector2<T>(x, y);
	}

	template<typename T>
	inline Vector3<T> Vector4<T>::ToVector3() const
	{
		return Vector3<T>(x, y, z);
	}

	template<typename T>
	inline Vector4<T> operator+(const Vector4<T>& aVector0, const Vector4<T>& aVector1)
	{
		return Vector4<T>(
			aVector0.x + aVector1.x,
			aVector0.y + aVector1.y,
			aVector0.z + aVector1.z,
			aVector0.w + aVector1.w);
	}

	template<typename T>
	inline Vector4<T> operator-(const Vector4<T>& aVector0, const Vector4<T>& aVector1)
	{
		return Vector4<T>(
			aVector0.x - aVector1.x,
			aVector0.y - aVector1.y,
			aVector0.z - aVector1.z,
			aVector0.w - aVector1.w);
	}

	template<typename T>
	inline Vector4<T> operator*(const Vector4<T>& aVector0, const Vector4<T>& aVector1)
	{
		return Vector4<T>(
			aVector0.x * aVector1.x,
			aVector0.y * aVector1.y,
			aVector0.z * aVector1.z,
			aVector0.w * aVector1.w);
	}

	template<typename T>
	inline Vector4<T> operator*(const Vector4<T>& aVector, const T& aScalar)
	{
		return Vector4<T>(
			aVector.x * aScalar,
			aVector.y * aScalar,
			aVector.z * aScalar,
			aVector.w * aScalar);
	}

	template<typename T>
	inline Vector4<T> operator*(const T& aScalar, const Vector4<T>& aVector)
	{
		return Vector4<T>(
			aVector.x * aScalar,
			aVector.y * aScalar,
			aVector.z * aScalar,
			aVector.w * aScalar);
	}

	template<typename T>
	inline Vector4<T> operator/(const Vector4<T>& aVector, const T& aScalar)
	{
		return Vector4<T>(
			aVector.x / aScalar,
			aVector.y / aScalar,
			aVector.z / aScalar,
			aVector.w / aScalar);
	}

	template<typename T>
	inline void operator+=(Vector4<T>& aVector0, const Vector4<T>& aVector1)
	{
		aVector0.x += aVector1.x;
		aVector0.y += aVector1.y;
		aVector0.z += aVector1.z;
		aVector0.w += aVector1.w;
	}

	template<typename T>
	inline void operator-=(Vector4<T>& aVector0, const Vector4<T>& aVector1)
	{
		aVector0.x -= aVector1.x;
		aVector0.y -= aVector1.y;
		aVector0.z -= aVector1.z;
		aVector0.w -= aVector1.w;
	}

	template<typename T>
	inline void operator*=(Vector4<T>& aVector, const T& aScalar)
	{
		aVector.x *= aScalar;
		aVector.y *= aScalar;
		aVector.z *= aScalar;
		aVector.w *= aScalar;
	}

	template<typename T>
	inline void operator/=(Vector4<T>& aVector, const T& aScalar)
	{
		aVector.x /= aScalar;
		aVector.y /= aScalar;
		aVector.z /= aScalar;
		aVector.w /= aScalar;
	}

	template<typename T>
	inline bool Vector4<T>::operator==(const Vector4<T>& anOther) const
	{
		return 	x == anOther.x &&
			y == anOther.y &&
			z == anOther.z &&
			w == anOther.w;
	}

	template<typename T>
	inline bool Vector4<T>::operator!=(const Vector4<T>& anOther) const
	{
		return !(*this == anOther);
	}

	template <typename T>
	inline std::ostream& operator<<(std::ostream& aOut, const Vector4<T>& aVector)
	{
		aOut << "(" << aVector.x << ", " << aVector.y << ", " << aVector.z << ", " << aVector.w << ")";
		return aOut;
	}
}
