#pragma once

#include "Maths.hpp"
#include "Matrix4x4.hpp"
#include "Quaternion.hpp"
#include "Vector3.hpp"

namespace CommonUtilities
{
	class Transform
	{
	public:
		Transform();
		Transform(const Vector3<float>& aTranslation, const Quaternion<float>& aRotation, const Vector3<float>& aScale);

		void SetPosition(const Vector3<float>& aTranslation);
		void SetRotation(const Quaternion<float>& aRotation);
		void SetRotation(float aYawDegrees, float aPitchDegrees, float aRollDegrees);
		void SetRotation(const Vector3<float>& aYawPitchRollDegrees);
		void SetScale(const Vector3<float>& aScale);
		void SetParent(Transform* aParent);

		const Vector3<float>& GetPosition() const;
		const Quaternion<float>& GetRotation() const;
		const Vector3<float>& GetScale() const;
		Transform* GetParent() const;

		void SetYawPitchRollRadians(float aYawRadians, float aPitchRadians, float aRollRadians);
		void SetYawPitchRollRadians(const Vector3<float>& aYawPitchRollRadians);

		const Matrix4f& GetLocalMatrix() const;
		Matrix4f GetWorldMatrix() const;

		Vector3<float> GetRight() const;
		Vector3<float> GetUp() const;
		Vector3<float> GetForward() const;

	private:
		void UpdateLocalMatrix() const;

		Vector3<float> myTranslation;
		Quaternion<float> myRotation;
		Vector3<float> myScale;
		mutable Matrix4f myLocalMatrix;
		mutable bool myDirty;
		Transform* myParent;
	};

	inline Transform::Transform()
		: myTranslation(Vector3<float>::Zero)
		, myRotation()
		, myScale(Vector3<float>::One)
		, myLocalMatrix()
		, myDirty(true)
		, myParent(nullptr)
	{
	}

	inline Transform::Transform(const Vector3<float>& aTranslation, const Quaternion<float>& aRotation, const Vector3<float>& aScale)
		: myTranslation(aTranslation)
		, myRotation(aRotation)
		, myScale(aScale)
		, myLocalMatrix()
		, myDirty(true)
		, myParent(nullptr)
	{
	}

	inline void Transform::SetPosition(const Vector3<float>& aTranslation)
	{
		myTranslation = aTranslation;
		myDirty = true;
	}

	inline void Transform::SetRotation(const Quaternion<float>& aRotation)
	{
		myRotation = aRotation;
		myDirty = true;
	}

	inline void Transform::SetRotation(float aYawDegrees, float aPitchDegrees, float aRollDegrees)
	{
		SetYawPitchRollRadians(
			Maths::DegreesToRadians(aYawDegrees),
			Maths::DegreesToRadians(aPitchDegrees),
			Maths::DegreesToRadians(aRollDegrees));
	}

	inline void Transform::SetRotation(const Vector3<float>& aYawPitchRollDegrees)
	{
		SetRotation(aYawPitchRollDegrees.x, aYawPitchRollDegrees.y, aYawPitchRollDegrees.z);
	}

	inline void Transform::SetScale(const Vector3<float>& aScale)
	{
		myScale = aScale;
		myDirty = true;
	}

	inline void Transform::SetParent(Transform* aParent)
	{
		myParent = aParent;
	}

	inline const Vector3<float>& Transform::GetPosition() const
	{
		return myTranslation;
	}

	inline const Quaternion<float>& Transform::GetRotation() const
	{
		return myRotation;
	}

	inline const Vector3<float>& Transform::GetScale() const
	{
		return myScale;
	}

	inline Transform* Transform::GetParent() const
	{
		return myParent;
	}

	inline void Transform::SetYawPitchRollRadians(float aYawRadians, float aPitchRadians, float aRollRadians)
	{
		myRotation = Quaternion<float>::CreateFromYawPitchRoll(aYawRadians, aPitchRadians, aRollRadians);
		myDirty = true;
	}

	inline void Transform::SetYawPitchRollRadians(const Vector3<float>& aYawPitchRollRadians)
	{
		SetYawPitchRollRadians(aYawPitchRollRadians.x, aYawPitchRollRadians.y, aYawPitchRollRadians.z);
	}

	inline const Matrix4f& Transform::GetLocalMatrix() const
	{
		if (myDirty)
		{
			UpdateLocalMatrix();
		}
		return myLocalMatrix;
	}

	inline Matrix4f Transform::GetWorldMatrix() const
	{
		Matrix4f local = GetLocalMatrix();
		if (myParent)
		{
			return local * myParent->GetWorldMatrix();
		}
		return local;
	}

	inline Vector3<float> Transform::GetRight() const
	{
		return myRotation.GetRight();
	}

	inline Vector3<float> Transform::GetUp() const
	{
		return myRotation.GetUp();
	}

	inline Vector3<float> Transform::GetForward() const
	{
		return myRotation.GetForward();
	}

	inline void Transform::UpdateLocalMatrix() const
	{
		Matrix4f scaleMatrix;
		scaleMatrix(1, 1) = myScale.x;
		scaleMatrix(2, 2) = myScale.y;
		scaleMatrix(3, 3) = myScale.z;

		Matrix4f rotationMatrix = myRotation.ToMatrix4x4();

		Matrix4f translationMatrix;
		translationMatrix(4, 1) = myTranslation.x;
		translationMatrix(4, 2) = myTranslation.y;
		translationMatrix(4, 3) = myTranslation.z;

		myLocalMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		myDirty = false;
	}
}
