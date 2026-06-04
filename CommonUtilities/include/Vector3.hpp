#pragma once
#include <cmath>
#include <ostream>

namespace Tga
{
	template <typename T>
	class Vector3;
	template <typename T>
	class Vector2;
	template <typename T>
	class Vector4;
}
namespace CommonUtilities
{
	template <typename T>
	class Vector2;
	template <typename T>
	class Vector4;

	template <typename T>
	class Vector3
	{
	public:
		T x; //Note: this variable is explicitly public.
		T y; //Note: this variable is explicitly public.
		T z; //Note: this variable is explicitly public.

		// Common constants
		static const Vector3<T> Zero;
		static const Vector3<T> One;
		static const Vector3<T> UnitX;
		static const Vector3<T> UnitY;
		static const Vector3<T> UnitZ;

		//Creates a null-vector
		Vector3();

		//Creates a vector (aX, aY, aZ)
		Vector3(const T& aX, const T& aY, const T& aZ);

		//Copy constructor (compiler generated)
		Vector3(const Vector3<T>& aVector) = default;

		//Assignment operator (compiler generated)
		Vector3<T>& operator=(const Vector3<T>& aVector3) = default;

		//Destructor (compiler generated)
		~Vector3() = default;

		// Returns a CommonUtilities Vector3 copy with another datatype as elements,
		// ex: converts from Vector3<int> to Vector3<float>
		// useful to explicitly do operations, such as addition, for an explicit type
		template<class TargetType>
		Vector3<TargetType> ToType() const;

		//Returns a copy of the vector as a Tga vector, use to interface with TGE
		Tga::Vector3<T> ToTga() const;

		//Returns a negated copy of the vector
		Vector3<T> operator-() const;

		//Returns the squared length of the vector, optimization compared to length
		T LengthSqr() const;

		//Returns the length of the vector, int vector is not required to work
		T Length() const;

		//Returns a normalized copy of this vector. Need not function for int vectors
		//Handle normalization of zero-vector by returning the zero vector
		Vector3<T> GetNormalized() const;

		//Normalizes the vector. Need not function for int vectors
		//Handle normalization of zero-vector by not modifying the vector.
		void Normalize();

		//Returns the dot product of this and aVector
		T Dot(const Vector3<T>& aVector) const;

		//Returns the cross product of this and aVector. Only for Vector3
		Vector3<T> Cross(const Vector3<T>& aVector) const;

		//Returns the distance between this vector and aOther
		T Distance(const Vector3<T>& aOther) const;

		//Returns the squared distance between this vector and aOther
		T DistanceSqr(const Vector3<T>& aOther) const;

		//Returns a linear interpolation between this vector and aOther at position aT (0 to 1)
		Vector3<T> Lerp(const Vector3<T>& aOther, T aT) const;

		//Returns the reflection of this vector around aNormal
		Vector3<T> Reflect(const Vector3<T>& aNormal) const;

		//Returns true if this vector is equal to aOther
		bool operator==(const Vector3<T>& aOther) const;

		//Returns true if this vector is not equal to aOther
		bool operator!=(const Vector3<T>& aOther) const;

		// Dimension changing helpers
		Vector2<T> ToVector2() const;
		Vector4<T> ToVector4(const T& aW = static_cast<T>(0)) const;
	};

	//Returns the vector sum of aVector0 and aVector1
	template <typename T>
	Vector3<T> operator+(const Vector3<T>& aVector0, const Vector3<T>& aVector1);

	//Returns the vector difference of aVector0 and aVector1
	template <typename T>
	Vector3<T> operator-(const Vector3<T>& aVector0, const Vector3<T>& aVector1);

	//Returns the vector aVector0 component-multiplied by aVector1
	template <typename T>
	Vector3<T> operator*(const Vector3<T>& aVector0, const Vector3<T>& aVector1);

	//Returns the vector aVector multiplied by the scalar aScalar. Vector * Scalar
	template <typename T>
	Vector3<T> operator*(const Vector3<T>& aVector, const T& aScalar);

	//Returns the vector aVector multiplied by the scalar aScalar. Scalar * Vector
	template <typename T>
	Vector3<T> operator*(const T& aScalar, const Vector3<T>& aVector);

	//Returns the vector aVector divided by the scalar aScalar
	template <typename T>
	Vector3<T> operator/(const Vector3<T>& aVector, const T& aScalar);

	//Equivalent to setting aVector0 to (aVector0 + aVector1)
	template <typename T>
	void operator+=(Vector3<T>& aVector0, const Vector3<T>& aVector1);

	//Equivalent to setting aVector0 to (aVector0 - aVector1)
	template <typename T>
	void operator-=(Vector3<T>& aVector0, const Vector3<T>& aVector1);

	//Equivalent to setting aVector to (aVector * aScalar)
	template <typename T>
	void operator*=(Vector3<T>& aVector, const T& aScalar);

	//Equivalent to setting aVector to (aVector / aScalar)
	template <typename T>
	void operator/=(Vector3<T>& aVector, const T& aScalar);

	template <typename T>
	std::ostream& operator<<(std::ostream& aOut, const Vector3<T>& aVector);

	//=====================================
	//  Implementations below this line
	//=====================================

	template<typename T>
	const Vector3<T> Vector3<T>::Zero = Vector3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));

	template<typename T>
	const Vector3<T> Vector3<T>::One = Vector3<T>(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1));

	template<typename T>
	const Vector3<T> Vector3<T>::UnitX = Vector3<T>(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));

	template<typename T>
	const Vector3<T> Vector3<T>::UnitY = Vector3<T>(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));

	template<typename T>
	const Vector3<T> Vector3<T>::UnitZ = Vector3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

	template <typename T>
	inline Vector3<T>::Vector3()
		: x(Vector3<T>::Zero.x)
		, y(Vector3<T>::Zero.y)
		, z(Vector3<T>::Zero.z)
	{
	}

	template <typename T>
	inline Vector3<T>::Vector3(const T& aX, const T& aY, const T& aZ)
		: x(aX)
		, y(aY)
		, z(aZ)
	{
	}

	template<typename T>
	inline Tga::Vector3<T> Vector3<T>::ToTga() const
	{
		return Tga::Vector3<T>(x, y, z);
	}

	template <typename T>
	inline Vector3<T> Vector3<T>::operator-() const
	{
		return Vector3<T>(-x, -y, -z);
	}

	template <typename T>
	inline T Vector3<T>::LengthSqr() const
	{
		return x * x +  y * y + z * z;
	}

	template <typename T>
	inline T Vector3<T>::Length() const
	{
		return static_cast<T>(std::sqrt(LengthSqr()));
	}

	template<typename T>
	inline Vector3<T> Vector3<T>::GetNormalized() const
	{
		const T zero = static_cast<T>(0);
		T len = Length();
		if (len == zero)
		{
			return Vector3<T>::Zero;
		}

		const T inv = static_cast<T>(1) / len;
		return Vector3<T>(x * inv, y * inv, z * inv);
	}

	template<typename T>
	inline void Vector3<T>::Normalize()
	{
		const T zero = static_cast<T>(0);
		T len = Length();
		if (len == zero)
		{
			x = Vector3<T>::Zero.x;
			y = Vector3<T>::Zero.y;
			z = Vector3<T>::Zero.z;
			return;
		}
		const T inv = static_cast<T>(1) / len;
		x *= inv;
		y *= inv;
		z *= inv;
	}

	template<typename T>
	inline T Vector3<T>::Dot(const Vector3<T>& aVector) const
	{
		return 	  this->x * aVector.x 
				+ this->y * aVector.y 
				+ this->z * aVector.z;
	}

	template<typename T>
	inline Vector3<T> Vector3<T>::Cross(const Vector3<T>& aVector) const
	{
		// Cross Product a × b = ( ay * bz - az * by, az * bx - ax * bz, ax * by - ay * bx )
		return Vector3<T>(
			this->y * aVector.z - this->z * aVector.y, 
			this->z * aVector.x - this->x * aVector.z, 
			this->x * aVector.y - this->y * aVector.x);
	}

	template<typename T>
	inline T Vector3<T>::Distance(const Vector3<T>& anOther) const
	{
		return (*this - anOther).Length();
	}

	template<typename T>
	inline T Vector3<T>::DistanceSqr(const Vector3<T>& anOther) const
	{
		return (*this - anOther).LengthSqr();
	}

	template<typename T>
	inline Vector3<T> Vector3<T>::Lerp(const Vector3<T>& anOther, T aT) const
	{
		return Vector3<T>(
			x + (anOther.x - x) * aT, 
			y + (anOther.y - y) * aT, 
			z + (anOther.z - z) * aT);
	}

	template<typename T>
	inline Vector3<T> Vector3<T>::Reflect(const Vector3<T>& aNormal) const
	{
		return *this - aNormal * (static_cast<T>(2) * this->Dot(aNormal));
	}

	template<typename T>
	inline bool Vector3<T>::operator==(const Vector3<T>& anOther) const
	{
		return 	x == anOther.x && 
				y == anOther.y && 
				z == anOther.z;
	}

	template<typename T>
	inline bool Vector3<T>::operator!=(const Vector3<T>& anOther) const
	{
		return !(*this == anOther);
	}

	
	template<typename T>
	template<class TargetType>
	inline Vector3<TargetType> Vector3<T>::ToType() const
	{
		return Vector3<TargetType>(
			static_cast<TargetType>(x),
			static_cast<TargetType>(y),
			static_cast<TargetType>(z));
	}

	template<typename T>
	Vector3<T> operator+(const Vector3<T>& aVector0, const Vector3<T>& aVector1)
	{
		// Addition a + b = (ax + bx, ay + by, az + bz)
		return Vector3<T>(
			aVector0.x + aVector1.x, 
			aVector0.y + aVector1.y, 
			aVector0.z + aVector1.z);
	}

	template<typename T>
	Vector3<T> operator-(const Vector3<T>& aVector0, const Vector3<T>& aVector1)
	{
		// Subtraction a − b = (ax − bx, ay − by, az − bz)
		return Vector3<T>(
			aVector0.x - aVector1.x, 
			aVector0.y - aVector1.y, 
			aVector0.z - aVector1.z);
	}

	template<typename T>
	Vector3<T> operator*(const Vector3<T>& aVector0, const Vector3<T>& aVector1)
	{
		// Component-wise Multiplication a * b = (ax * bx, ay * by, az * bz) 
		return Vector3<T>(
			aVector0.x * aVector1.x, 
			aVector0.y * aVector1.y, 
			aVector0.z * aVector1.z);
	}

	template<typename T>
	Vector3<T> operator*(const Vector3<T>& aVector, const T& aScalar)
	{
		// Scalar Multiplication s * a = (s * ax, s * ay, s * az) a * s is the same.
		return Vector3<T>(
			aScalar * aVector.x, 
			aScalar * aVector.y, 
			aScalar * aVector.z);
	}

	template<typename T>
	Vector3<T> operator*(const T& aScalar, const Vector3<T>& aVector)
	{
		return Vector3<T>(
			aScalar * aVector.x, 
			aScalar * aVector.y, 
			aScalar * aVector.z);
	}

	template<typename T>
	Vector3<T> operator/(const Vector3<T>& aVector, const T& aScalar)
	{
		// Scalar Division a / s = (ax / s, ay / s, az / s)  (Assuming s isnt 0.)
		return Vector3<T>(
			aVector.x / aScalar, 
			aVector.y / aScalar, 
			aVector.z / aScalar);
	}

	template<typename T>
	void operator+=(Vector3<T>& aVector0, const Vector3<T>& aVector1)
	{
		aVector0.x += aVector1.x;
		aVector0.y += aVector1.y;
		aVector0.z += aVector1.z;
	}

	template<typename T>
	void operator-=(Vector3<T>& aVector0, const Vector3<T>& aVector1)
	{
		aVector0.x -= aVector1.x;
		aVector0.y -= aVector1.y;
		aVector0.z -= aVector1.z;
	}

	template<typename T>
	void operator*=(Vector3<T>& aVector, const T& aScalar)
	{
		aVector.x *= aScalar;
		aVector.y *= aScalar;
		aVector.z *= aScalar;
	}

	template<typename T>
	void operator/=(Vector3<T>& aVector, const T& aScalar)
	{
		aVector.x /= aScalar;
		aVector.y /= aScalar;
		aVector.z /= aScalar;
	}

	template <typename T>
	inline std::ostream& operator<<(std::ostream& aOut, const Vector3<T>& aVector)
	{
		aOut << "(" << aVector.x << ", " << aVector.y << ", " << aVector.z << ")";
		return aOut;
	}

	// (dimension-changing helper implementations are located earlier in this file)

}