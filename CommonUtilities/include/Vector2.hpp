#pragma once
#include <cmath>
#include <ostream>

namespace Tga
{
	template <typename T>
	class Vector2;
}

namespace CommonUtilities
{
	template <typename T>
	class Vector3;
	template <typename T>
	class Vector4;

	template <typename T>
	class Vector2
	{
	public:
		T x;
		T y;

		// Common constants
		static const Vector2<T> Zero;
		static const Vector2<T> One;
		static const Vector2<T> UnitX;
		static const Vector2<T> UnitY;

		Vector2();
		Vector2(const T& aX, const T& aY);
		Vector2(const Vector2<T>&) = default;
		Vector2& operator=(const Vector2<T>&) = default;
		~Vector2() = default;

		template<class TargetType>
		Vector2<TargetType> ToType() const;
		Tga::Vector2<T> ToTga() const;

		// Dimension changing helpers
		Vector3<T> ToVector3(const T& aZ = static_cast<T>(0)) const;
		Vector4<T> ToVector4(const T& aZ = static_cast<T>(0), const T& aW = static_cast<T>(0)) const;

		Vector2<T> operator-() const;
		T LengthSqr() const;
		T Length() const;
		Vector2<T> GetNormalized() const;
		void Normalize();
		T Dot(const Vector2<T>& aVector) const;

		T Distance(const Vector2<T>& anOther) const;
		T DistanceSqr(const Vector2<T>& anOther) const;
		Vector2<T> Lerp(const Vector2<T>& anOther, T aT) const;
		Vector2<T> Reflect(const Vector2<T>& aNormal) const;

		bool operator==(const Vector2<T>& anOther) const;
		bool operator!=(const Vector2<T>& anOther) const;
	};

	template <typename T> 
	Vector2<T> operator+(const Vector2<T>& aVector0, const Vector2<T>& aVector1);

	template <typename T> 
	Vector2<T> operator-(const Vector2<T>& aVector0, const Vector2<T>& aVector1);
	
	template <typename T> 
	Vector2<T> operator*(const Vector2<T>& aVector0, const Vector2<T>& aVector1);
	
	template <typename T> 
	Vector2<T> operator*(const Vector2<T>& aVector, const T& aScalar);
	
	template <typename T> 
	Vector2<T> operator*(const T& aScalar, const Vector2<T>& aVector);
	
	template <typename T> 
	Vector2<T> operator/(const Vector2<T>& aVector, const T& aScalar);
	
	template <typename T> 
	void operator+=(Vector2<T>& aVector0, const Vector2<T>& aVector1);
	
	template <typename T> 
	void operator-=(Vector2<T>& aVector0, const Vector2<T>& aVector1);
	
	template <typename T> 
	void operator*=(Vector2<T>& aVector, const T& aScalar);
	
	template <typename T> 
	void operator/=(Vector2<T>& aVector, const T& aScalar);

	template <typename T>
	std::ostream& operator<<(std::ostream& aOut, const Vector2<T>& aVector);

	//================
	// Implementations
	//================

	template<typename T>
	const Vector2<T> Vector2<T>::Zero = Vector2<T>(static_cast<T>(0), static_cast<T>(0));

	template<typename T>
	const Vector2<T> Vector2<T>::One = Vector2<T>(static_cast<T>(1), static_cast<T>(1));

	template<typename T>
	const Vector2<T> Vector2<T>::UnitX = Vector2<T>(static_cast<T>(1), static_cast<T>(0));

	template<typename T>
	const Vector2<T> Vector2<T>::UnitY = Vector2<T>(static_cast<T>(0), static_cast<T>(1));
	
	template<typename T>
	inline Vector2<T>::Vector2()
	{
		x = static_cast<T>(0);
		y = static_cast<T>(0);
	}

	template<typename T>
	inline Vector2<T>::Vector2(const T& aX, const T& aY)
	{
		x = aX;
		y = aY;
	}

	template<typename T>
	inline Tga::Vector2<T> Vector2<T>::ToTga() const
	{
		return Tga::Vector2<T>(x, y);
	}

	template<typename T>
	inline Vector2<T> Vector2<T>::operator-() const
	{
		return Vector2<T>(-x, -y);
	}

	template<typename T>
	inline T Vector2<T>::LengthSqr() const
	{
		return x * x + y * y;
	}

	template<typename T>
	inline T Vector2<T>::Length() const
	{
		return static_cast<T>(std::sqrt(LengthSqr()));
	}

	template<typename T>
	inline Vector2<T> Vector2<T>::GetNormalized() const
	{
		const T zero = static_cast<T>(0);
		T len = Length();
		if (len == zero)
		{
			return Vector2<T>::Zero;
		}
		const T inv = static_cast<T>(1) / len;
		return Vector2<T>(x * inv, y * inv);
	}

	template<typename T>
	inline void Vector2<T>::Normalize()
	{
		const T zero = static_cast<T>(0);
		T len = Length();
		if (len == zero)
		{
			x = Vector2<T>::Zero.x;
			y = Vector2<T>::Zero.y;
			return;
		}
		const T inv = static_cast<T>(1) / len;
		x *= inv;
		y *= inv;
	}

	template<typename T>
	inline T Vector2<T>::Dot(const Vector2<T>& aVector) const
	{
		return x * aVector.x + y * aVector.y;
	}

	template<typename T>
	inline T Vector2<T>::Distance(const Vector2<T>& anOther) const
	{
		return (*this - anOther).Length();
	}

	template<typename T>
	inline T Vector2<T>::DistanceSqr(const Vector2<T>& anOther) const
	{
		return (*this - anOther).LengthSqr();
	}

	template<typename T>
	inline Vector2<T> Vector2<T>::Lerp(const Vector2<T>& anOther, T aT) const
	{
		return Vector2<T>(x + (anOther.x - x) * aT, y + (anOther.y - y) * aT);
	}

	template<typename T>
	inline Vector2<T> Vector2<T>::Reflect(const Vector2<T>& aNormal) const
	{
		return *this - aNormal * (static_cast<T>(2) * this->Dot(aNormal));
	}

	template<typename T>
	inline bool Vector2<T>::operator==(const Vector2<T>& anOther) const
	{
		return x == anOther.x && y == anOther.y;
	}

	template<typename T>
	inline bool Vector2<T>::operator!=(const Vector2<T>& anOther) const
	{
		return !(*this == anOther);
	}

	template<typename T>
	inline Vector2<T> operator+(const Vector2<T>& aVector0, const Vector2<T>& aVector1)
	{
		return Vector2<T>(
			aVector0.x + aVector1.x, 
			aVector0.y + aVector1.y);
	}

	template<typename T>
	inline Vector2<T> operator-(const Vector2<T>& aVector0, const Vector2<T>& aVector1)
	{
		return Vector2<T>(
			aVector0.x - aVector1.x, 
			aVector0.y - aVector1.y);
	}

	template<typename T>
	inline Vector2<T> operator*(const Vector2<T>& aVector0, const Vector2<T>& aVector1)
	{
		return Vector2<T>(
			aVector0.x * aVector1.x, 
			aVector0.y * aVector1.y);
	}

	template<typename T>
	inline Vector2<T> operator*(const Vector2<T>& aVector, const T& aScalar)
	{
		return Vector2<T>(
			aVector.x * aScalar, 
			aVector.y * aScalar);
	}

	template<typename T>
	inline Vector2<T> operator*(const T& aScalar, const Vector2<T>& aVector)
	{
		return Vector2<T>(
			aVector.x * aScalar, 
			aVector.y * aScalar);
	}

	template<typename T>
	inline Vector2<T> operator/(const Vector2<T>& aVector, const T& aScalar)
	{
		return Vector2<T>(
			aVector.x / aScalar, 
			aVector.y / aScalar);
	}

	template<typename T>
	inline void operator+=(Vector2<T>& aVector0, const Vector2<T>& aVector1)
	{
		aVector0.x += aVector1.x;
		aVector0.y += aVector1.y;
	}

	template<typename T>
	inline void operator-=(Vector2<T>& aVector0, const Vector2<T>& aVector1)
	{
		aVector0.x -= aVector1.x;
		aVector0.y -= aVector1.y;
	}

	template<typename T>
	inline void operator*=(Vector2<T>& aVector, const T& aScalar)
	{
		aVector.x *= aScalar;
		aVector.y *= aScalar;
	}

	template<typename T>
	inline void operator/=(Vector2<T>& aVector, const T& aScalar)
	{
		aVector.x /= aScalar;
		aVector.y /= aScalar;
	}

	template<typename T>
	template<class TargetType>
	inline Vector2<TargetType> Vector2<T>::ToType() const
	{
		return Vector2<TargetType>(static_cast<TargetType>(x), static_cast<TargetType>(y));
	}

	template <typename T>
	inline std::ostream& operator<<(std::ostream& aOut, const Vector2<T>& aVector)
	{
		aOut << "(" << aVector.x << ", " << aVector.y << ")";
		return aOut;
	}

	template<typename T>
	inline Vector3<T> Vector2<T>::ToVector3(const T& aZ) const
	{
		return Vector3<T>(x, y, aZ);
	}

	template<typename T>
	inline Vector4<T> Vector2<T>::ToVector4(const T& aZ, const T& aW) const
	{
		return Vector4<T>(x, y, aZ, aW);
	}
}
