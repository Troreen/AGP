#pragma once

#include "Matrix4x4.hpp"
#include "Quaternion.hpp"
#include "Vector3.hpp"

namespace CommonUtilities
{
	template <typename T>
	class Transform
	{
	public:
		Transform();
		Transform(const Vector3<T>& aTranslation, const Quaternion<T>& aRotation, const Vector3<T>& aScale);

		void SetPosition(const Vector3<T>& aTranslation);
		void SetRotation(const Quaternion<T>& aRotation);
		void SetScale(const Vector3<T>& aScale);
		void SetParent(Transform<T>* aParent);

		const Vector3<T>& GetPosition() const;
		const Quaternion<T>& GetRotation() const;
		const Vector3<T>& GetScale() const;
		Transform<T>* GetParent() const;

		// Vector components are (yaw, pitch, roll) in radians.
		void SetYawPitchRollRadians(const T aYawRadians, const T aPitchRadians, const T aRollRadians);
		void SetYawPitchRollRadians(const Vector3<T>& aYawPitchRollRadians);

		const Matrix4x4<T>& GetLocalMatrix();
		Matrix4x4<T> GetWorldMatrix();

		Vector3<T> GetRight() const;
		Vector3<T> GetUp() const;
		Vector3<T> GetForward() const;

	private:
		void UpdateLocalMatrix();

		Vector3<T> myTranslation;
		Quaternion<T> myRotation;
		Vector3<T> myScale;
		Matrix4x4<T> myLocalMatrix;
		bool myDirty;
		Transform<T>* myParent;
	};

	// --- Implementation ---

	template <typename T>
	inline Transform<T>::Transform()
		: myTranslation(Vector3<T>::Zero)
		, myRotation()
		, myScale(Vector3<T>::One)
		, myLocalMatrix()
		, myDirty(true)
		, myParent(nullptr)
	{
	}

	template <typename T>
	inline Transform<T>::Transform(const Vector3<T>& aTranslation, const Quaternion<T>& aRotation, const Vector3<T>& aScale)
		: myTranslation(aTranslation)
		, myRotation(aRotation)
		, myScale(aScale)
		, myLocalMatrix()
		, myDirty(true)
		, myParent(nullptr)
	{
	}

	template <typename T>
	inline void Transform<T>::SetPosition(const Vector3<T>& aTranslation)
	{
		myTranslation = aTranslation;
		myDirty = true;
	}

	template <typename T>
	inline void Transform<T>::SetRotation(const Quaternion<T>& aRotation)
	{
		myRotation = aRotation;
		myDirty = true;
	}

	template <typename T>
	inline void Transform<T>::SetScale(const Vector3<T>& aScale)
	{
		myScale = aScale;
		myDirty = true;
	}

	template <typename T>
	inline void Transform<T>::SetParent(Transform<T>* aParent)
	{
		myParent = aParent;
	}

	template <typename T>
	inline const Vector3<T>& Transform<T>::GetPosition() const
	{
		return myTranslation;
	}

	template <typename T>
	inline const Quaternion<T>& Transform<T>::GetRotation() const
	{
		return myRotation;
	}

	template <typename T>
	inline const Vector3<T>& Transform<T>::GetScale() const
	{
		return myScale;
	}

	template <typename T>
	inline Transform<T>* Transform<T>::GetParent() const
	{
		return myParent;
	}

	template <typename T>
	inline void Transform<T>::SetYawPitchRollRadians(const T aYawRadians, const T aPitchRadians, const T aRollRadians)
	{
		myRotation = Quaternion<T>::CreateFromYawPitchRoll(aYawRadians, aPitchRadians, aRollRadians);
		myDirty = true;
	}

	template <typename T>
	inline void Transform<T>::SetYawPitchRollRadians(const Vector3<T>& aYawPitchRollRadians)
	{
		SetYawPitchRollRadians(aYawPitchRollRadians.x, aYawPitchRollRadians.y, aYawPitchRollRadians.z);
	}

	template <typename T>
	inline const Matrix4x4<T>& Transform<T>::GetLocalMatrix()
	{
		if (myDirty)
		{
			UpdateLocalMatrix();
		}
		return myLocalMatrix;
	}

	template <typename T>
	inline Matrix4x4<T> Transform<T>::GetWorldMatrix()
	{
		Matrix4x4<T> local = GetLocalMatrix();
		if (myParent)
		{
			return local * myParent->GetWorldMatrix();
		}
		return local;
	}

	template <typename T>
	inline Vector3<T> Transform<T>::GetRight() const
	{
		return myRotation.GetRight();
	}

	template <typename T>
	inline Vector3<T> Transform<T>::GetUp() const
	{
		return myRotation.GetUp();
	}

	template <typename T>
	inline Vector3<T> Transform<T>::GetForward() const
	{
		return myRotation.GetForward();
	}

	template <typename T>
	inline void Transform<T>::UpdateLocalMatrix()
	{
		Matrix4x4<T> scaleMatrix;
		scaleMatrix(1, 1) = myScale.x;
		scaleMatrix(2, 2) = myScale.y;
		scaleMatrix(3, 3) = myScale.z;

		Matrix4x4<T> rotationMatrix = myRotation.ToMatrix4x4();

		Matrix4x4<T> translationMatrix;
		translationMatrix(4, 1) = myTranslation.x;
		translationMatrix(4, 2) = myTranslation.y;
		translationMatrix(4, 3) = myTranslation.z;

		myLocalMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		myDirty = false;
	}
}
