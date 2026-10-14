#pragma once

#include <cmath>
#include <initializer_list>
#include <ostream>
#include <stdexcept>

#include "Vector3.hpp"
#include "Vector4.hpp"

namespace CommonUtilities
{
	template <typename T>
	class Matrix3x3;

	template <typename T>
	class Matrix4x4
	{
	public:
		static const Matrix4x4 Identity;

		// Creates the identity matrix.
		Matrix4x4();

		// Fill constructor (row-major): { 16 elements }
		Matrix4x4(std::initializer_list<T> aElements);

		// Nested list constructor (row-major): { {4}, {4}, {4}, {4} }
		Matrix4x4(std::initializer_list<std::initializer_list<T>> aRows);

		// Copies the 3x3 matrix into the top left area of the 4x4 matrix.
		Matrix4x4(const Matrix3x3<T>& aMatrix);
		
		// Copy Constructor.
		Matrix4x4(const Matrix4x4& aMatrix) = default;
		
		//Assignment operator (compiler generated)
		Matrix4x4& operator=(const Matrix4x4& aMatrix) = default;
		
		// () operator for accessing element (row, column) for read/write or read, respectively.
		T& operator()(const int aRow, const int aColumn);
		const T& operator()(const int aRow, const int aColumn) const;

		// Index access (zero-based row): allows m[row][col]
		T* operator[](const int aRow);
		const T* operator[](const int aRow) const;

		// Row and column access use the same one-based indexing as operator(row, column).
		Vector4<T> GetRow(const int aRow) const;
		Vector4<T> GetColumn(const int aColumn) const;
		Vector3<T> GetRight() const;
		Vector3<T> GetUp() const;
		Vector3<T> GetForward() const;
		
		// Creates a transposed copy of the matrix.
		Matrix4x4<T> GetTranspose() const;

		Matrix4x4<T> GetInverse() const;

		// Assumes the matrix is made up of nothing but rotations and translations.
		Matrix4x4<T> GetFastInverse() const;

		// Returns the inverse-transpose of the upper-left 3x3 with translation cleared.
		Matrix4x4<T> GetInverseTranspose3x3() const;

		// Static functions for creating rotation matrices.
		static Matrix4x4<T> CreateRotationAroundX(const T aAngleInRadians);
		static Matrix4x4<T> CreateRotationAroundY(const T aAngleInRadians);
		static Matrix4x4<T> CreateRotationAroundZ(const T aAngleInRadians);
		static Matrix4x4<T> FromTranslation(const T& aX, const T& aY, const T& aZ);
		static Matrix4x4<T> FromRotation(const T& aPitch, const T& aYaw, const T& aRoll);
		static Matrix4x4<T> LookAt(const Vector3<T>& aEyePoint, const Vector3<T>& aTarget, const Vector3<T>& aUp);
		static Matrix4x4<T> CreateOrthographicProjection(const T& aWidth, const T& aHeight, const T& aNear, const T& aFar);
		static Matrix4x4<T> CreateOrthographicProjection(const T& aLeft, const T& aRight, const T& aBottom, const T& aTop, const T& aNear, const T& aFar);
		static Matrix4x4<T> CreatePerspectiveProjection(const T& aWidth, const T& aHeight, const T& aHorizontalFov, const T& aNear, const T& aFar);
		static Matrix4x4<T> CreateNDCToTextureSpace();

		// Compound assignment operators
		Matrix4x4& operator+=(const Matrix4x4& aMatrix);
		Matrix4x4& operator-=(const Matrix4x4& aMatrix);
		Matrix4x4& operator*=(const Matrix4x4& aMatrix);
		Matrix4x4& operator*=(const T& aScalar);

		bool operator==(const Matrix4x4& aOther) const;
		bool operator!=(const Matrix4x4& aOther) const { return !(*this == aOther); }
	
	private:
		union
		{
			T myGridValues[4][4];
			T myValues[16];
		};
	};

	// Free function operators
	template <typename T>
	Matrix4x4<T> operator+(const Matrix4x4<T>& aMatrix0, const Matrix4x4<T>& aMatrix1);

	template <typename T>
	Matrix4x4<T> operator-(const Matrix4x4<T>& aMatrix0, const Matrix4x4<T>& aMatrix1);

	template <typename T>
	Matrix4x4<T> operator*(const Matrix4x4<T>& aMatrix0, const Matrix4x4<T>& aMatrix1);

	template <typename T>
	Matrix4x4<T> operator*(const T& aScalar, const Matrix4x4<T>& aMatrix);

	template <typename T>
	Matrix4x4<T> operator*(const Matrix4x4<T>& aMatrix, const T& aScalar);

	// Vector * Matrix (row-vector convention): v' = v * M
	template <typename T>
	Vector4<T> operator*(const Vector4<T>& aVector, const Matrix4x4<T>& aMatrix);

	template <typename T>
	std::ostream& operator<<(std::ostream& aStream, const Matrix4x4<T>& aMatrix);

	// --- Implementation ---

	template<typename T>
	const Matrix4x4<T> Matrix4x4<T>::Identity = Matrix4x4<T>();

	template <typename T>
	inline Matrix4x4<T>::Matrix4x4() : myValues()
    {
        std::memset(myValues, 0, 16 * sizeof(T));
        constexpr T one = static_cast<T>(1);
        myGridValues[0][0] = one;
        myGridValues[1][1] = one;
        myGridValues[2][2] = one;
        myGridValues[3][3] = one;
    }

	template <typename T>
	inline Matrix4x4<T>::Matrix4x4(const Matrix3x3<T>& aMatrix) : myValues()
	{
		std::memset(myValues, 0, 16 * sizeof(T));

		constexpr size_t rowLength = sizeof(T) * 3;
		std::memcpy(myValues, &aMatrix(1, 1), rowLength);
		std::memcpy(myValues + 4, &aMatrix(2, 1), rowLength);
		std::memcpy(myValues + 8, &aMatrix(3, 1), rowLength);
		myGridValues[3][3] = static_cast<T>(1);
	}

	template <typename T>
	inline Matrix4x4<T>::Matrix4x4(std::initializer_list<T> aElements) : myValues()
	{
		if (aElements.size() != 16)
		{
			throw std::invalid_argument("Matrix4x4 requires exactly 16 elements");
		}

		std::memcpy(myValues, aElements.begin(), sizeof(T) * 16);
	}

	template <typename T>
	inline Matrix4x4<T>::Matrix4x4(std::initializer_list<std::initializer_list<T>> aRows)
	{
		if (aRows.size() != 4)
		{
			throw std::invalid_argument("Matrix4x4 requires exactly 4 rows");
		}

		int r = 0;
		for (const auto& row : aRows)
		{
			if (row.size() != 4)
			{
				throw std::invalid_argument("Matrix4x4 requires exactly 4 columns per row");
			}

			int c = 0;
			for (const auto& value : row)
			{
				myGridValues[r][c++] = value;
			}
			++r;
		}
	}

	template <typename T>
	inline T& Matrix4x4<T>::operator()(const int aRow, const int aColumn)
	{
		if (aRow < 1 || aRow > 4 || aColumn < 1 || aColumn > 4)
		{
			throw std::out_of_range("Matrix4x4 index out of range");
		}

		return myGridValues[aRow - 1][aColumn - 1];
	}

	template <typename T>
	inline const T& Matrix4x4<T>::operator()(const int aRow, const int aColumn) const
	{
		if (aRow < 1 || aRow > 4 || aColumn < 1 || aColumn > 4)
		{
			throw std::out_of_range("Matrix4x4 index out of range");
		}

		return myGridValues[aRow - 1][aColumn - 1];
	}

	template <typename T>
	inline T* Matrix4x4<T>::operator[](const int aRow)
	{
		if (aRow < 0 || aRow >= 4)
		{
			throw std::out_of_range("Matrix4x4 row index out of range");
		}

		return myGridValues[aRow];
	}

	template <typename T>
	inline const T* Matrix4x4<T>::operator[](const int aRow) const
	{
		if (aRow < 0 || aRow >= 4)
		{
			throw std::out_of_range("Matrix4x4 row index out of range");
		}

		return myGridValues[aRow];
	}

	template <typename T>
	inline Vector4<T> Matrix4x4<T>::GetRow(const int aRow) const
	{
		if (aRow < 1 || aRow > 4)
		{
			throw std::out_of_range("Matrix4x4 row index out of range");
		}

		return Vector4<T>((*this)(aRow, 1), (*this)(aRow, 2), (*this)(aRow, 3), (*this)(aRow, 4));
	}

	template <typename T>
	inline Vector4<T> Matrix4x4<T>::GetColumn(const int aColumn) const
	{
		if (aColumn < 1 || aColumn > 4)
		{
			throw std::out_of_range("Matrix4x4 column index out of range");
		}

		return Vector4<T>((*this)(1, aColumn), (*this)(2, aColumn), (*this)(3, aColumn), (*this)(4, aColumn));
	}

	template <typename T>
	inline Vector3<T> Matrix4x4<T>::GetRight() const
	{
		return Vector3<T>((*this)(1, 1), (*this)(1, 2), (*this)(1, 3));
	}

	template <typename T>
	inline Vector3<T> Matrix4x4<T>::GetUp() const
	{
		return Vector3<T>((*this)(2, 1), (*this)(2, 2), (*this)(2, 3));
	}

	template <typename T>
	inline Vector3<T> Matrix4x4<T>::GetForward() const
	{
		return Vector3<T>((*this)(3, 1), (*this)(3, 2), (*this)(3, 3));
	}

	template <typename T>
	inline Matrix4x4<T> Matrix4x4<T>::GetTranspose() const
	{
		return Matrix4x4{
			myGridValues[0][0], myGridValues[1][0], myGridValues[2][0], myGridValues[3][0],
			myGridValues[0][1], myGridValues[1][1], myGridValues[2][1], myGridValues[3][1],
			myGridValues[0][2], myGridValues[1][2], myGridValues[2][2], myGridValues[3][2],
			myGridValues[0][3], myGridValues[1][3], myGridValues[2][3], myGridValues[3][3]
		};
	}

	template<typename T>
	inline Matrix4x4<T> Matrix4x4<T>::GetInverse() const
	{
		T inv[16]{};

		inv[0] = myValues[5] * myValues[10] * myValues[15] -
			myValues[5] * myValues[11] * myValues[14] -
			myValues[9] * myValues[6] * myValues[15] +
			myValues[9] * myValues[7] * myValues[14] +
			myValues[13] * myValues[6] * myValues[11] -
			myValues[13] * myValues[7] * myValues[10];

		inv[4] = -myValues[4] * myValues[10] * myValues[15] +
			myValues[4] * myValues[11] * myValues[14] +
			myValues[8] * myValues[6] * myValues[15] -
			myValues[8] * myValues[7] * myValues[14] -
			myValues[12] * myValues[6] * myValues[11] +
			myValues[12] * myValues[7] * myValues[10];

		inv[8] = myValues[4] * myValues[9] * myValues[15] -
			myValues[4] * myValues[11] * myValues[13] -
			myValues[8] * myValues[5] * myValues[15] +
			myValues[8] * myValues[7] * myValues[13] +
			myValues[12] * myValues[5] * myValues[11] -
			myValues[12] * myValues[7] * myValues[9];

		inv[12] = -myValues[4] * myValues[9] * myValues[14] +
			myValues[4] * myValues[10] * myValues[13] +
			myValues[8] * myValues[5] * myValues[14] -
			myValues[8] * myValues[6] * myValues[13] -
			myValues[12] * myValues[5] * myValues[10] +
			myValues[12] * myValues[6] * myValues[9];

		inv[1] = -myValues[1] * myValues[10] * myValues[15] +
			myValues[1] * myValues[11] * myValues[14] +
			myValues[9] * myValues[2] * myValues[15] -
			myValues[9] * myValues[3] * myValues[14] -
			myValues[13] * myValues[2] * myValues[11] +
			myValues[13] * myValues[3] * myValues[10];

		inv[5] = myValues[0] * myValues[10] * myValues[15] -
			myValues[0] * myValues[11] * myValues[14] -
			myValues[8] * myValues[2] * myValues[15] +
			myValues[8] * myValues[3] * myValues[14] +
			myValues[12] * myValues[2] * myValues[11] -
			myValues[12] * myValues[3] * myValues[10];

		inv[9] = -myValues[0] * myValues[9] * myValues[15] +
			myValues[0] * myValues[11] * myValues[13] +
			myValues[8] * myValues[1] * myValues[15] -
			myValues[8] * myValues[3] * myValues[13] -
			myValues[12] * myValues[1] * myValues[11] +
			myValues[12] * myValues[3] * myValues[9];

		inv[13] = myValues[0] * myValues[9] * myValues[14] -
			myValues[0] * myValues[10] * myValues[13] -
			myValues[8] * myValues[1] * myValues[14] +
			myValues[8] * myValues[2] * myValues[13] +
			myValues[12] * myValues[1] * myValues[10] -
			myValues[12] * myValues[2] * myValues[9];

		inv[2] = myValues[1] * myValues[6] * myValues[15] -
			myValues[1] * myValues[7] * myValues[14] -
			myValues[5] * myValues[2] * myValues[15] +
			myValues[5] * myValues[3] * myValues[14] +
			myValues[13] * myValues[2] * myValues[7] -
			myValues[13] * myValues[3] * myValues[6];

		inv[6] = -myValues[0] * myValues[6] * myValues[15] +
			myValues[0] * myValues[7] * myValues[14] +
			myValues[4] * myValues[2] * myValues[15] -
			myValues[4] * myValues[3] * myValues[14] -
			myValues[12] * myValues[2] * myValues[7] +
			myValues[12] * myValues[3] * myValues[6];

		inv[10] = myValues[0] * myValues[5] * myValues[15] -
			myValues[0] * myValues[7] * myValues[13] -
			myValues[4] * myValues[1] * myValues[15] +
			myValues[4] * myValues[3] * myValues[13] +
			myValues[12] * myValues[1] * myValues[7] -
			myValues[12] * myValues[3] * myValues[5];

		inv[14] = -myValues[0] * myValues[5] * myValues[14] +
			myValues[0] * myValues[6] * myValues[13] +
			myValues[4] * myValues[1] * myValues[14] -
			myValues[4] * myValues[2] * myValues[13] -
			myValues[12] * myValues[1] * myValues[6] +
			myValues[12] * myValues[2] * myValues[5];

		inv[3] = -myValues[1] * myValues[6] * myValues[11] +
			myValues[1] * myValues[7] * myValues[10] +
			myValues[5] * myValues[2] * myValues[11] -
			myValues[5] * myValues[3] * myValues[10] -
			myValues[9] * myValues[2] * myValues[7] +
			myValues[9] * myValues[3] * myValues[6];

		inv[7] = myValues[0] * myValues[6] * myValues[11] -
			myValues[0] * myValues[7] * myValues[10] -
			myValues[4] * myValues[2] * myValues[11] +
			myValues[4] * myValues[3] * myValues[10] +
			myValues[8] * myValues[2] * myValues[7] -
			myValues[8] * myValues[3] * myValues[6];

		inv[11] = -myValues[0] * myValues[5] * myValues[11] +
			myValues[0] * myValues[7] * myValues[9] +
			myValues[4] * myValues[1] * myValues[11] -
			myValues[4] * myValues[3] * myValues[9] -
			myValues[8] * myValues[1] * myValues[7] +
			myValues[8] * myValues[3] * myValues[5];

		inv[15] = myValues[0] * myValues[5] * myValues[10] -
			myValues[0] * myValues[6] * myValues[9] -
			myValues[4] * myValues[1] * myValues[10] +
			myValues[4] * myValues[2] * myValues[9] +
			myValues[8] * myValues[1] * myValues[6] -
			myValues[8] * myValues[2] * myValues[5];

		T det = myValues[0] * inv[0] + myValues[1] * inv[4] + myValues[2] * inv[8] + myValues[3] * inv[12];

		//if (det == 0)
		//	return false;

		det = T(1.0) / det;

		Matrix4x4<T> returnMatrix;
		for (int i = 0; i < 16; ++i)
		{
			returnMatrix.myValues[i] = inv[i] * det;
		}

		return returnMatrix;
	}

	template <typename T>
	inline Matrix4x4<T> Matrix4x4<T>::GetFastInverse() const
	{
		Matrix4x4<T> result = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			-myGridValues[3][0], -myGridValues[3][1], -myGridValues[3][2], 1
		};

		result *= {
			myGridValues[0][0], myGridValues[1][0], myGridValues[2][0], 0,
				myGridValues[0][1], myGridValues[1][1], myGridValues[2][1], 0,
				myGridValues[0][2], myGridValues[1][2], myGridValues[2][2], 0,
				0, 0, 0, 1
		};

		return result;
	}

	template <typename T>
	inline Matrix4x4<T> Matrix4x4<T>::GetInverseTranspose3x3() const
	{
		const T m00 = myGridValues[0][0];
		const T m01 = myGridValues[0][1];
		const T m02 = myGridValues[0][2];
		const T m10 = myGridValues[1][0];
		const T m11 = myGridValues[1][1];
		const T m12 = myGridValues[1][2];
		const T m20 = myGridValues[2][0];
		const T m21 = myGridValues[2][1];
		const T m22 = myGridValues[2][2];

		const T determinant =
			m00 * (m11 * m22 - m12 * m21) -
			m01 * (m10 * m22 - m12 * m20) +
			m02 * (m10 * m21 - m11 * m20);

		Matrix4x4<T> result;
		if (std::abs(determinant) <= static_cast<T>(0.000001))
		{
			return result;
		}

		const T invDet = static_cast<T>(1) / determinant;
		const T inverse[3][3] =
		{
			{
				(m11 * m22 - m12 * m21) * invDet,
				(m02 * m21 - m01 * m22) * invDet,
				(m01 * m12 - m02 * m11) * invDet
			},
			{
				(m12 * m20 - m10 * m22) * invDet,
				(m00 * m22 - m02 * m20) * invDet,
				(m02 * m10 - m00 * m12) * invDet
			},
			{
				(m10 * m21 - m11 * m20) * invDet,
				(m01 * m20 - m00 * m21) * invDet,
				(m00 * m11 - m01 * m10) * invDet
			}
		};

		for (int row = 0; row < 3; ++row)
		{
			for (int column = 0; column < 3; ++column)
			{
				result(row + 1, column + 1) = inverse[column][row];
			}
		}

		result(1, 4) = static_cast<T>(0);
		result(2, 4) = static_cast<T>(0);
		result(3, 4) = static_cast<T>(0);
		result(4, 1) = static_cast<T>(0);
		result(4, 2) = static_cast<T>(0);
		result(4, 3) = static_cast<T>(0);
		result(4, 4) = static_cast<T>(1);
		return result;
	}

	template <typename T>
	inline Matrix4x4<T> Matrix4x4<T>::CreateRotationAroundX(const T aAngleInRadians)
	{
		Matrix4x4<T> rotationMatrix;
		const T cosAngle = static_cast<T>(std::cos(aAngleInRadians));
		const T sinAngle = static_cast<T>(std::sin(aAngleInRadians));
		rotationMatrix(2, 2) = cosAngle;
		rotationMatrix(2, 3) = sinAngle;
		rotationMatrix(3, 2) = -sinAngle;
		rotationMatrix(3, 3) = cosAngle;
		return rotationMatrix;
	}

	template <typename T>
	inline Matrix4x4<T> Matrix4x4<T>::CreateRotationAroundY(const T aAngleInRadians)
	{
		Matrix4x4<T> rotationMatrix;
		const T cosAngle = static_cast<T>(std::cos(aAngleInRadians));
		const T sinAngle = static_cast<T>(std::sin(aAngleInRadians));
		rotationMatrix(1, 1) = cosAngle;
		rotationMatrix(1, 3) = -sinAngle;
		rotationMatrix(3, 1) = sinAngle;
		rotationMatrix(3, 3) = cosAngle;
		return rotationMatrix;
	}

	template <typename T>
	inline Matrix4x4<T> Matrix4x4<T>::CreateRotationAroundZ(const T aAngleInRadians)
	{
		Matrix4x4<T> rotationMatrix;
		const T cosAngle = static_cast<T>(std::cos(aAngleInRadians));
		const T sinAngle = static_cast<T>(std::sin(aAngleInRadians));
		rotationMatrix(1, 1) = cosAngle;
		rotationMatrix(1, 2) = sinAngle;
		rotationMatrix(2, 1) = -sinAngle;
		rotationMatrix(2, 2) = cosAngle;
		return rotationMatrix;
	}

	template<typename T>
	inline Matrix4x4<T> Matrix4x4<T>::FromTranslation(const T& aX, const T& aY, const T& aZ)
	{
		return {
			1,  0,  0,  0,
			0,  1,  0,  0,
			0,  0,  1,  0,
			aX, aY, aZ, 1
		};
	}

	template<typename T>
	inline Matrix4x4<T> Matrix4x4<T>::FromRotation(const T& aPitch, const T& aYaw, const T& aRoll)
	{
		Matrix4x4<T> toReturn = CreateRotationAroundZ(aRoll);
		toReturn *= CreateRotationAroundX(aPitch);
		toReturn *= CreateRotationAroundY(aYaw);
		return toReturn;
	}

	template<typename T>
	inline Matrix4x4<T> Matrix4x4<T>::LookAt(const Vector3<T>& aEyePoint, const Vector3<T>& aTarget, const Vector3<T>& aUp)
	{
		Vector3<T> forward = aTarget - aEyePoint;
		forward.Normalize();
		Vector3<T> right = aUp.Cross(forward);
		right.Normalize();
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
		view(4, 1) = -aEyePoint.Dot(right);
		view(4, 2) = -aEyePoint.Dot(up);
		view(4, 3) = -aEyePoint.Dot(forward);
		return view;
	}

	template<typename T>
	inline Matrix4x4<T> Matrix4x4<T>::CreateOrthographicProjection(const T& aWidth, const T& aHeight, const T& aNear, const T& aFar)
	{
		Matrix4x4<T> projection;
		projection(1, 1) = static_cast<T>(2) / aWidth;
		projection(2, 2) = static_cast<T>(2) / aHeight;
		projection(3, 3) = static_cast<T>(1) / (aFar - aNear);
		projection(4, 3) = aNear / (aNear - aFar);

		return projection;
	}

	template<typename T>
	inline Matrix4x4<T> Matrix4x4<T>::CreateOrthographicProjection(const T& aLeft, const T& aRight, const T& aBottom, const T& aTop, const T& aNear, const T& aFar)
	{
		Matrix4x4<T> projection;
		const T width = aRight - aLeft;
		const T height = aTop - aBottom;
		const T depth = aFar - aNear;
		projection(1, 1) = static_cast<T>(2) / width;
		projection(2, 2) = static_cast<T>(2) / height;
		projection(3, 3) = static_cast<T>(1) / depth;
		projection(4, 1) = -(aRight + aLeft) / width;
		projection(4, 2) = -(aTop + aBottom) / height;
		projection(4, 3) = aNear / (aNear - aFar);
		return projection;
	}

	template<typename T>
	inline Matrix4x4<T> Matrix4x4<T>::CreatePerspectiveProjection(const T& aWidth, const T& aHeight, const T& aHorizontalFov, const T& aNear, const T& aFar)
	{
		const float hFoVRad = static_cast<float>(aHorizontalFov);
		const float vFoVRad = 2.0f * std::atan(std::tan(hFoVRad * 0.5f) * (static_cast<float>(aHeight) / static_cast<float>(aWidth)));

		const float xScale = 1.0f / std::tanf(hFoVRad * 0.5f);
		const float yScale = 1.0f / std::tanf(vFoVRad * 0.5f);
		const T zRange = aFar / (aFar - aNear);

		Matrix4x4<T> projection;
		projection(1, 1) = xScale;
		projection(2, 2) = yScale;
		projection(3, 3) = zRange;
		projection(3, 4) = static_cast<T>(1);
		projection(4, 3) = -zRange * aNear;
		projection(4, 4) = static_cast<T>(0);

		return projection;
	}

	template<typename T>
	inline Matrix4x4<T> Matrix4x4<T>::CreateNDCToTextureSpace()
	{
		constexpr T zero = static_cast<T>(0);
		constexpr T pointFive = static_cast<T>(0.5f);
		constexpr T one = static_cast<T>(1);

		return Matrix4x4<T>{
			pointFive, zero, zero, zero,
			zero, -pointFive, zero, zero,
			zero, zero, one, zero,
			pointFive, pointFive, zero, one
		};
	}

	// Compound operators
	template <typename T>
	inline Matrix4x4<T>& Matrix4x4<T>::operator+=(const Matrix4x4<T>& aMatrix)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			myValues[i] += aMatrix.myValues[i];
		}
		return *this;
	}

	template <typename T>
	inline Matrix4x4<T>& Matrix4x4<T>::operator-=(const Matrix4x4<T>& aMatrix)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			myValues[i] -= aMatrix.myValues[i];
		}
		return *this;
	}

	template <typename T>
	inline Matrix4x4<T>& Matrix4x4<T>::operator*=(const Matrix4x4<T>& aMatrix)
	{
		*this = (*this) * aMatrix;
		return *this;
	}

	template <typename T>
	inline Matrix4x4<T>& Matrix4x4<T>::operator*=(const T& aScalar)
	{
		for (size_t i = 0; i < 16; ++i)
		{
			myValues[i] *= aScalar;
		}
		return *this;
	}

	template <typename T>
	inline bool Matrix4x4<T>::operator==(const Matrix4x4<T>& aOther) const
	{
		for (size_t i = 0; i < 16; ++i)
		{
			if (!(myValues[i] == aOther.myValues[i]))
			{
				return false;
			}
		}
		return true;
	}

	// Free function implementations
	template <typename T>
	inline Matrix4x4<T> operator+(const Matrix4x4<T>& aMatrix0, const Matrix4x4<T>& aMatrix1)
	{
		Matrix4x4<T> result = aMatrix0;
		result += aMatrix1;
		return result;
	}

	template <typename T>
	inline Matrix4x4<T> operator-(const Matrix4x4<T>& aMatrix0, const Matrix4x4<T>& aMatrix1)
	{
		Matrix4x4<T> result = aMatrix0;
		result -= aMatrix1;
		return result;
	}

	template <typename T>
	inline Matrix4x4<T> operator*(const Matrix4x4<T>& aMatrixA, const Matrix4x4<T>& aMatrixB)
	{
		Matrix4x4<T> result;

		// Row 1
		result(1, 1) = aMatrixA(1, 1) * aMatrixB(1, 1)
			+ aMatrixA(1, 2) * aMatrixB(2, 1)
			+ aMatrixA(1, 3) * aMatrixB(3, 1)
			+ aMatrixA(1, 4) * aMatrixB(4, 1);

		result(1, 2) = aMatrixA(1, 1) * aMatrixB(1, 2)
			+ aMatrixA(1, 2) * aMatrixB(2, 2)
			+ aMatrixA(1, 3) * aMatrixB(3, 2)
			+ aMatrixA(1, 4) * aMatrixB(4, 2);

		result(1, 3) = aMatrixA(1, 1) * aMatrixB(1, 3)
			+ aMatrixA(1, 2) * aMatrixB(2, 3)
			+ aMatrixA(1, 3) * aMatrixB(3, 3)
			+ aMatrixA(1, 4) * aMatrixB(4, 3);

		result(1, 4) = aMatrixA(1, 1) * aMatrixB(1, 4)
			+ aMatrixA(1, 2) * aMatrixB(2, 4)
			+ aMatrixA(1, 3) * aMatrixB(3, 4)
			+ aMatrixA(1, 4) * aMatrixB(4, 4);

		// Row 2
		result(2, 1) = aMatrixA(2, 1) * aMatrixB(1, 1)
			+ aMatrixA(2, 2) * aMatrixB(2, 1)
			+ aMatrixA(2, 3) * aMatrixB(3, 1)
			+ aMatrixA(2, 4) * aMatrixB(4, 1);

		result(2, 2) = aMatrixA(2, 1) * aMatrixB(1, 2)
			+ aMatrixA(2, 2) * aMatrixB(2, 2)
			+ aMatrixA(2, 3) * aMatrixB(3, 2)
			+ aMatrixA(2, 4) * aMatrixB(4, 2);

		result(2, 3) = aMatrixA(2, 1) * aMatrixB(1, 3)
			+ aMatrixA(2, 2) * aMatrixB(2, 3)
			+ aMatrixA(2, 3) * aMatrixB(3, 3)
			+ aMatrixA(2, 4) * aMatrixB(4, 3);

		result(2, 4) = aMatrixA(2, 1) * aMatrixB(1, 4)
			+ aMatrixA(2, 2) * aMatrixB(2, 4)
			+ aMatrixA(2, 3) * aMatrixB(3, 4)
			+ aMatrixA(2, 4) * aMatrixB(4, 4);

		// Row 3
		result(3, 1) = aMatrixA(3, 1) * aMatrixB(1, 1)
			+ aMatrixA(3, 2) * aMatrixB(2, 1)
			+ aMatrixA(3, 3) * aMatrixB(3, 1)
			+ aMatrixA(3, 4) * aMatrixB(4, 1);

		result(3, 2) = aMatrixA(3, 1) * aMatrixB(1, 2)
			+ aMatrixA(3, 2) * aMatrixB(2, 2)
			+ aMatrixA(3, 3) * aMatrixB(3, 2)
			+ aMatrixA(3, 4) * aMatrixB(4, 2);

		result(3, 3) = aMatrixA(3, 1) * aMatrixB(1, 3)
			+ aMatrixA(3, 2) * aMatrixB(2, 3)
			+ aMatrixA(3, 3) * aMatrixB(3, 3)
			+ aMatrixA(3, 4) * aMatrixB(4, 3);

		result(3, 4) = aMatrixA(3, 1) * aMatrixB(1, 4)
			+ aMatrixA(3, 2) * aMatrixB(2, 4)
			+ aMatrixA(3, 3) * aMatrixB(3, 4)
			+ aMatrixA(3, 4) * aMatrixB(4, 4);

		// Row 4
		result(4, 1) = aMatrixA(4, 1) * aMatrixB(1, 1)
			+ aMatrixA(4, 2) * aMatrixB(2, 1)
			+ aMatrixA(4, 3) * aMatrixB(3, 1)
			+ aMatrixA(4, 4) * aMatrixB(4, 1);

		result(4, 2) = aMatrixA(4, 1) * aMatrixB(1, 2)
			+ aMatrixA(4, 2) * aMatrixB(2, 2)
			+ aMatrixA(4, 3) * aMatrixB(3, 2)
			+ aMatrixA(4, 4) * aMatrixB(4, 2);

		result(4, 3) = aMatrixA(4, 1) * aMatrixB(1, 3)
			+ aMatrixA(4, 2) * aMatrixB(2, 3)
			+ aMatrixA(4, 3) * aMatrixB(3, 3)
			+ aMatrixA(4, 4) * aMatrixB(4, 3);

		result(4, 4) = aMatrixA(4, 1) * aMatrixB(1, 4)
			+ aMatrixA(4, 2) * aMatrixB(2, 4)
			+ aMatrixA(4, 3) * aMatrixB(3, 4)
			+ aMatrixA(4, 4) * aMatrixB(4, 4);

		return result;
	}

	template <typename T>
	inline Matrix4x4<T> operator*(const T& aScalar, const Matrix4x4<T>& aMatrix)
	{
		Matrix4x4<T> result = aMatrix;
		result *= aScalar;
		return result;
	}

	template <typename T>
	inline Matrix4x4<T> operator*(const Matrix4x4<T>& aMatrix, const T& aScalar)
	{
		Matrix4x4<T> result = aMatrix;
		result *= aScalar;
		return result;
	}

	template <typename T>
	inline Vector4<T> operator*(const Vector4<T>& aVector, const Matrix4x4<T>& aMatrix)
	{
		return Vector4<T>(
			aVector.x * aMatrix(1, 1) + aVector.y * aMatrix(2, 1) + aVector.z * aMatrix(3, 1) + aVector.w * aMatrix(4, 1),
			aVector.x * aMatrix(1, 2) + aVector.y * aMatrix(2, 2) + aVector.z * aMatrix(3, 2) + aVector.w * aMatrix(4, 2),
			aVector.x * aMatrix(1, 3) + aVector.y * aMatrix(2, 3) + aVector.z * aMatrix(3, 3) + aVector.w * aMatrix(4, 3),
			aVector.x * aMatrix(1, 4) + aVector.y * aMatrix(2, 4) + aVector.z * aMatrix(3, 4) + aVector.w * aMatrix(4, 4));
	}

	template <typename T>
	inline std::ostream& operator<<(std::ostream& aStream, const Matrix4x4<T>& aMatrix)
	{
		for (int r = 0; r < 4; ++r)
		{
			aStream << "[";
			for (int c = 0; c < 4; ++c)
			{
				aStream << aMatrix[r][c];
				if (c < 3)
				{
					aStream << ", ";
				}
			}
			aStream << "]";
			if (r < 3)
			{
				aStream << "\n";
			}
		}
		return aStream;
	}

	using Matrix4f = Matrix4x4<float>;
}
