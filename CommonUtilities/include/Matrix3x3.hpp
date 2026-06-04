#pragma once 

#include <cmath>
#include <initializer_list>
#include <ostream>
#include <stdexcept>

#include "Vector3.hpp"

namespace CommonUtilities
{
	template <typename T>
	class Matrix4x4;
	
	template <typename T>
	class Matrix3x3
	{
	public:
		// Creates the identity matrix.
		Matrix3x3();

		// Fill constructor (row-major): { m00,m01,m02, m10,m11,m12, m20,m21,m22 }
		Matrix3x3(std::initializer_list<T> aElements);

		// Nested list constructor (row-major): { {m00,m01,m02}, {m10,m11,m12}, {m20,m21,m22} }
		Matrix3x3(std::initializer_list<std::initializer_list<T>> aRows);

		// Copy Constructor.
		Matrix3x3(const Matrix3x3& aMatrix) = default;
		
		//Assignment operator (compiler generated)
		Matrix3x3& operator=(const Matrix3x3& aMatrix) = default;
		
		// Copies the top left 3x3 part of the Matrix4x4.
		Matrix3x3(const Matrix4x4<T>& aMatrix);
		
		// () operator for accessing element (row, column) for read/write or read, respectively. 
		T& operator()(const int aRow, const int aColumn);
		const T& operator()(const int aRow, const int aColumn) const;

		// Index access (zero-based row): allows m[row][col]
		T* operator[](const int aRow);
		const T* operator[](const int aRow) const;

		// Row and column access use the same one-based indexing as operator(row, column).
		Vector3<T> GetRow(const int aRow) const;
		Vector3<T> GetColumn(const int aColumn) const;
		Vector3<T> GetRight() const;
		Vector3<T> GetUp() const;
		Vector3<T> GetForward() const;
		
		// Creates a transposed copy of the matrix.
		Matrix3x3 GetTranspose() const;
		
		// Static functions for creating rotation matrices.
		static Matrix3x3 CreateRotationAroundX(const T aAngleInRadians);
		static Matrix3x3 CreateRotationAroundY(const T aAngleInRadians);
		static Matrix3x3 CreateRotationAroundZ(const T aAngleInRadians);

		// Compound assignment operators
		Matrix3x3& operator+=(const Matrix3x3& aMatrix);
		Matrix3x3& operator-=(const Matrix3x3& aMatrix);
		Matrix3x3& operator*=(const Matrix3x3& aMatrix);
		Matrix3x3& operator*=(const T& aScalar);

		bool operator==(const Matrix3x3& aOther) const;
		bool operator!=(const Matrix3x3& aOther) const { return !(*this == aOther); }

	private:
		T myMatrix[3][3];
	};	 

	// Free function operators
	template <typename T>
	Matrix3x3<T> operator+(const Matrix3x3<T>& aMatrix0, const Matrix3x3<T>& aMatrix1);

	template <typename T>
	Matrix3x3<T> operator-(const Matrix3x3<T>& aMatrix0, const Matrix3x3<T>& aMatrix1);

	template <typename T>
	Matrix3x3<T> operator*(const Matrix3x3<T>& aMatrix0, const Matrix3x3<T>& aMatrix1);

	template <typename T>
	Matrix3x3<T> operator*(const T& aScalar, const Matrix3x3<T>& aMatrix);
	
	template <typename T>
	Matrix3x3<T> operator*(const Matrix3x3<T>& aMatrix, const T& aScalar);

	// Vector * Matrix (row-vector convention): v' = v * M
	template <typename T>
	Vector3<T> operator*(const Vector3<T>& aVector, const Matrix3x3<T>& aMatrix);

	template <typename T>
	std::ostream& operator<<(std::ostream& aStream, const Matrix3x3<T>& aMatrix);

	// --- Implementation ---

	template <typename T>
	inline Matrix3x3<T>::Matrix3x3()
	{
		for (int row = 0; row < 3; ++row)
		{
			for (int col = 0; col < 3; ++col)
			{
				myMatrix[row][col] = (row == col) ? static_cast<T>(1) : static_cast<T>(0);
			}
		}
	}

	template <typename T>
	inline Matrix3x3<T>::Matrix3x3(const Matrix4x4<T>& aMatrix)
	{
		for (int row = 0; row < 3; ++row)
		{
			for (int col = 0; col < 3; ++col)
			{
				myMatrix[row][col] = aMatrix(row + 1, col + 1);
			}
		}
	}

	template <typename T>
	inline Matrix3x3<T>::Matrix3x3(std::initializer_list<T> aElements)
	{
		if (aElements.size() != 9)
		{
			throw std::invalid_argument("Matrix3x3 requires exactly 9 elements");
		}

		auto it = aElements.begin();
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				myMatrix[r][c] = *it++;
			}
		}
	}

	template <typename T>
	inline Matrix3x3<T>::Matrix3x3(std::initializer_list<std::initializer_list<T>> aRows)
	{
		if (aRows.size() != 3)
		{
			throw std::invalid_argument("Matrix3x3 requires exactly 3 rows");
		}

		int r = 0;
		for (const auto& row : aRows)
		{
			if (row.size() != 3)
			{
				throw std::invalid_argument("Matrix3x3 requires exactly 3 columns per row");
			}

			int c = 0;
			for (const auto& value : row)
			{
				myMatrix[r][c++] = value;
			}
			++r;
		}
	}

	template <typename T>
	inline T& Matrix3x3<T>::operator()(const int aRow, const int aColumn)
	{
		if (aRow < 1 || aRow > 3 || aColumn < 1 || aColumn > 3)
		{
			throw std::out_of_range("Matrix3x3 index out of range");
		}

		return myMatrix[aRow - 1][aColumn - 1];
	}

	template <typename T>
	inline const T& Matrix3x3<T>::operator()(const int aRow, const int aColumn) const
	{
		if (aRow < 1 || aRow > 3 || aColumn < 1 || aColumn > 3)
		{
			throw std::out_of_range("Matrix3x3 index out of range");
		}

		return myMatrix[aRow - 1][aColumn - 1];
	}
	
	template <typename T>
	inline T* Matrix3x3<T>::operator[](const int aRow)
	{
		if (aRow < 0 || aRow >= 3)
		{
			throw std::out_of_range("Matrix3x3 row index out of range");
		}

		return myMatrix[aRow];
	}

	template <typename T>
	inline const T* Matrix3x3<T>::operator[](const int aRow) const
	{
		if (aRow < 0 || aRow >= 3)
		{
			throw std::out_of_range("Matrix3x3 row index out of range");
		}

		return myMatrix[aRow];
	}

	template <typename T>
	inline Vector3<T> Matrix3x3<T>::GetRow(const int aRow) const
	{
		if (aRow < 1 || aRow > 3)
		{
			throw std::out_of_range("Matrix3x3 row index out of range");
		}

		return Vector3<T>((*this)(aRow, 1), (*this)(aRow, 2), (*this)(aRow, 3));
	}

	template <typename T>
	inline Vector3<T> Matrix3x3<T>::GetColumn(const int aColumn) const
	{
		if (aColumn < 1 || aColumn > 3)
		{
			throw std::out_of_range("Matrix3x3 column index out of range");
		}

		return Vector3<T>((*this)(1, aColumn), (*this)(2, aColumn), (*this)(3, aColumn));
	}

	template <typename T>
	inline Vector3<T> Matrix3x3<T>::GetRight() const
	{
		return GetRow(1);
	}

	template <typename T>
	inline Vector3<T> Matrix3x3<T>::GetUp() const
	{
		return GetRow(2);
	}

	template <typename T>
	inline Vector3<T> Matrix3x3<T>::GetForward() const
	{
		return GetRow(3);
	}

	template <typename T>
	inline Matrix3x3<T> Matrix3x3<T>::GetTranspose() const
	{
		Matrix3x3<T> transposedMatrix;
		for (int row = 0; row < 3; ++row)
		{
			for (int col = 0; col < 3; ++col)
			{
				transposedMatrix.myMatrix[col][row] = myMatrix[row][col];
			}
		}
		return transposedMatrix;
	}

	template <typename T>
	inline Matrix3x3<T> Matrix3x3<T>::CreateRotationAroundX(const T aAngleInRadians)
	{
		Matrix3x3<T> rotationMatrix;
		T cosAngle = static_cast<T>(std::cos(aAngleInRadians));
		T sinAngle = static_cast<T>(std::sin(aAngleInRadians));
		rotationMatrix(2, 2) = cosAngle;
		rotationMatrix(2, 3) = sinAngle;
		rotationMatrix(3, 2) = -sinAngle;
		rotationMatrix(3, 3) = cosAngle;
		return rotationMatrix;
	}

	template <typename T>
	inline Matrix3x3<T> Matrix3x3<T>::CreateRotationAroundY(const T aAngleInRadians)
	{
		Matrix3x3<T> rotationMatrix;
		T cosAngle = static_cast<T>(std::cos(aAngleInRadians));
		T sinAngle = static_cast<T>(std::sin(aAngleInRadians));
		rotationMatrix(1, 1) = cosAngle;
		rotationMatrix(1, 3) = -sinAngle;
		rotationMatrix(3, 1) = sinAngle;
		rotationMatrix(3, 3) = cosAngle;
		return rotationMatrix;
	}

	template <typename T>
	inline Matrix3x3<T> Matrix3x3<T>::CreateRotationAroundZ(const T aAngleInRadians)
	{
		Matrix3x3<T> rotationMatrix;
		T cosAngle = static_cast<T>(std::cos(aAngleInRadians));
		T sinAngle = static_cast<T>(std::sin(aAngleInRadians));
		rotationMatrix(1, 1) = cosAngle;
		rotationMatrix(1, 2) = sinAngle;
		rotationMatrix(2, 1) = -sinAngle;
		rotationMatrix(2, 2) = cosAngle;
		return rotationMatrix;
	}

	// Compound operators
	template <typename T>
	inline Matrix3x3<T>& Matrix3x3<T>::operator+=(const Matrix3x3<T>& aMatrix)
	{
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				myMatrix[r][c] += aMatrix.myMatrix[r][c];
			}
		}
		return *this;
	}

	template <typename T>
	inline Matrix3x3<T>& Matrix3x3<T>::operator-=(const Matrix3x3<T>& aMatrix)
	{
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				myMatrix[r][c] -= aMatrix.myMatrix[r][c];
			}
		}
		return *this;
	}

	template <typename T>
	inline Matrix3x3<T>& Matrix3x3<T>::operator*=(const Matrix3x3<T>& aMatrix)
	{
		Matrix3x3<T> result;
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				T sum = static_cast<T>(0);
				for (int k = 0; k < 3; ++k)
				{
					sum += myMatrix[r][k] * aMatrix.myMatrix[k][c];
				}
				result.myMatrix[r][c] = sum;
			}
		}
		*this = result;
		return *this;
	}

	template <typename T>
	inline Matrix3x3<T>& Matrix3x3<T>::operator*=(const T& aScalar)
	{
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				myMatrix[r][c] *= aScalar;
			}
		}
		return *this;
	}

	template <typename T>
	inline bool Matrix3x3<T>::operator==(const Matrix3x3<T>& aOther) const
	{
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				if (myMatrix[r][c] != aOther.myMatrix[r][c])
				{
					return false;
				}
			}
		}
		return true;
	}

	template <typename T>
	inline Matrix3x3<T> operator+(const Matrix3x3<T>& aMatrix0, const Matrix3x3<T>& aMatrix1)
	{
		Matrix3x3<T> result = aMatrix0;
		result += aMatrix1;
		return result;
	}

	template <typename T>
	inline Matrix3x3<T> operator-(const Matrix3x3<T>& aMatrix0, const Matrix3x3<T>& aMatrix1)
	{
		Matrix3x3<T> result = aMatrix0;
		result -= aMatrix1;
		return result;
	}

	template <typename T>
	inline Matrix3x3<T> operator*(const Matrix3x3<T>& aMatrix0, const Matrix3x3<T>& aMatrix1)
	{
		Matrix3x3<T> result;
		for (int r = 1; r <= 3; ++r)
		{
			for (int c = 1; c <= 3; ++c)
			{
				T sum = static_cast<T>(0);
				for (int k = 1; k <= 3; ++k)
				{
					sum += aMatrix0(r, k) * aMatrix1(k, c);
				}
				result(r, c) = sum;
			}
		}
		return result;
	}

	template <typename T>
	inline Matrix3x3<T> operator*(const T& aScalar, const Matrix3x3<T>& aMatrix)
	{
		Matrix3x3<T> result = aMatrix;
		result *= aScalar;
		return result;
	}
	
	template <typename T>
	inline Matrix3x3<T> operator*(const Matrix3x3<T>& aMatrix, const T& aScalar)
	{
		Matrix3x3<T> result = aMatrix;
		result *= aScalar;
		return result;
	}

	template <typename T>
	inline Vector3<T> operator*(const Vector3<T>& aVector, const Matrix3x3<T>& aMatrix)
	{
		return Vector3<T>(
			aVector.x * aMatrix(1, 1) + aVector.y * aMatrix(2, 1) + aVector.z * aMatrix(3, 1),
			aVector.x * aMatrix(1, 2) + aVector.y * aMatrix(2, 2) + aVector.z * aMatrix(3, 2),
			aVector.x * aMatrix(1, 3) + aVector.y * aMatrix(2, 3) + aVector.z * aMatrix(3, 3));
	}

	template <typename T>
	inline std::ostream& operator<<(std::ostream& aStream, const Matrix3x3<T>& aMatrix)
	{
		for (int r = 0; r < 3; ++r)
		{
			aStream << "[";
			for (int c = 0; c < 3; ++c)
			{
				aStream << aMatrix[r][c];
				if (c < 2)
				{
					aStream << ", ";
				}
			}
			aStream << "]";
			if (r < 2)
			{
				aStream << "\n";
			}
		}
		return aStream;
	}

}
