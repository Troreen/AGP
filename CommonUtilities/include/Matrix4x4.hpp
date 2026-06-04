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

		// Assumes the matrix is made up of nothing but rotations and translations.
		Matrix4x4<T> GetFastInverse() const;

		// Static functions for creating rotation matrices.
		static Matrix4x4 CreateRotationAroundX(const T aAngleInRadians);
		static Matrix4x4 CreateRotationAroundY(const T aAngleInRadians);
		static Matrix4x4 CreateRotationAroundZ(const T aAngleInRadians);

		// Compound assignment operators
		Matrix4x4& operator+=(const Matrix4x4& aMatrix);
		Matrix4x4& operator-=(const Matrix4x4& aMatrix);
		Matrix4x4& operator*=(const Matrix4x4& aMatrix);
		Matrix4x4& operator*=(const T& aScalar);

		bool operator==(const Matrix4x4& aOther) const;
		bool operator!=(const Matrix4x4& aOther) const { return !(*this == aOther); }
	
	private:
		T myMatrix[4][4];
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

	template <typename T>
	inline Matrix4x4<T>::Matrix4x4()
	{
		for (int row = 0; row < 4; ++row)
		{
			for (int col = 0; col < 4; ++col)
			{
				myMatrix[row][col] = (row == col) ? static_cast<T>(1) : static_cast<T>(0);
			}
		}
	}

	template <typename T>
	inline Matrix4x4<T>::Matrix4x4(const Matrix3x3<T>& aMatrix)
	{
		for (int row = 0; row < 3; ++row)
		{
			for (int col = 0; col < 3; ++col)
			{
				myMatrix[row][col] = aMatrix(row + 1, col + 1);
			}
		}

		for (int i = 0; i < 4; ++i)
		{
			myMatrix[i][3] = (i == 3) ? static_cast<T>(1) : static_cast<T>(0);
			myMatrix[3][i] = (i == 3) ? static_cast<T>(1) : static_cast<T>(0);
		}
	}

	template <typename T>
	inline Matrix4x4<T>::Matrix4x4(std::initializer_list<T> aElements)
	{
		if (aElements.size() != 16)
		{
			throw std::invalid_argument("Matrix4x4 requires exactly 16 elements");
		}

		auto it = aElements.begin();
		for (int r = 0; r < 4; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				myMatrix[r][c] = *it++;
			}
		}
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
				myMatrix[r][c++] = value;
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

		return myMatrix[aRow - 1][aColumn - 1];
	}

	template <typename T>
	inline const T& Matrix4x4<T>::operator()(const int aRow, const int aColumn) const
	{
		if (aRow < 1 || aRow > 4 || aColumn < 1 || aColumn > 4)
		{
			throw std::out_of_range("Matrix4x4 index out of range");
		}

		return myMatrix[aRow - 1][aColumn - 1];
	}

	template <typename T>
	inline T* Matrix4x4<T>::operator[](const int aRow)
	{
		if (aRow < 0 || aRow >= 4)
		{
			throw std::out_of_range("Matrix4x4 row index out of range");
		}

		return myMatrix[aRow];
	}

	template <typename T>
	inline const T* Matrix4x4<T>::operator[](const int aRow) const
	{
		if (aRow < 0 || aRow >= 4)
		{
			throw std::out_of_range("Matrix4x4 row index out of range");
		}

		return myMatrix[aRow];
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
		Matrix4x4<T> transposedMatrix;
		for (int row = 0; row < 4; ++row)
		{
			for (int col = 0; col < 4; ++col)
			{
				transposedMatrix.myMatrix[col][row] = myMatrix[row][col];
			}
		}
		return transposedMatrix;
	}

	template <typename T>
	inline Matrix4x4<T> Matrix4x4<T>::GetFastInverse() const
	{
		Matrix4x4<T> inverseMatrix;

		// Transpose the rotation part (upper-left 3x3)
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				inverseMatrix(r + 1, c + 1) = myMatrix[c][r];
			}
		}

		// Invert translation for row-vector convention (translation is stored in row 4, columns 1..3).
		Vector4<T> translation(-myMatrix[3][0], -myMatrix[3][1], -myMatrix[3][2], static_cast<T>(1));
		Vector4<T> invertedTranslation = translation * inverseMatrix;

		inverseMatrix(4, 1) = invertedTranslation.x;
		inverseMatrix(4, 2) = invertedTranslation.y;
		inverseMatrix(4, 3) = invertedTranslation.z;
		inverseMatrix(1, 4) = static_cast<T>(0);
		inverseMatrix(2, 4) = static_cast<T>(0);
		inverseMatrix(3, 4) = static_cast<T>(0);
		inverseMatrix(4, 4) = static_cast<T>(1);

		return inverseMatrix;
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

	// Compound operators
	template <typename T>
	inline Matrix4x4<T>& Matrix4x4<T>::operator+=(const Matrix4x4<T>& aMatrix)
	{
		for (int r = 0; r < 4; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				myMatrix[r][c] += aMatrix.myMatrix[r][c];
			}
		}
		return *this;
	}

	template <typename T>
	inline Matrix4x4<T>& Matrix4x4<T>::operator-=(const Matrix4x4<T>& aMatrix)
	{
		for (int r = 0; r < 4; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				myMatrix[r][c] -= aMatrix.myMatrix[r][c];
			}
		}
		return *this;
	}

	template <typename T>
	inline Matrix4x4<T>& Matrix4x4<T>::operator*=(const Matrix4x4<T>& aMatrix)
	{
		Matrix4x4<T> result;
		for (int r = 0; r < 4; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				T sum = static_cast<T>(0);
				for (int k = 0; k < 4; ++k)
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
	inline Matrix4x4<T>& Matrix4x4<T>::operator*=(const T& aScalar)
	{
		for (int r = 0; r < 4; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				myMatrix[r][c] *= aScalar;
			}
		}
		return *this;
	}

	template <typename T>
	inline bool Matrix4x4<T>::operator==(const Matrix4x4<T>& aOther) const
	{
		for (int r = 0; r < 4; ++r)
		{
			for (int c = 0; c < 4; ++c)
			{
				if (myMatrix[r][c] != aOther.myMatrix[r][c])
				{
					return false;
				}
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
	inline Matrix4x4<T> operator*(const Matrix4x4<T>& aMatrix0, const Matrix4x4<T>& aMatrix1)
	{
		Matrix4x4<T> result;
		for (int r = 1; r <= 4; ++r)
		{
			for (int c = 1; c <= 4; ++c)
			{
				T sum = static_cast<T>(0);
				for (int k = 1; k <= 4; ++k)
				{
					sum += aMatrix0(r, k) * aMatrix1(k, c);
				}
				result(r, c) = sum;
			}
		}
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
}
