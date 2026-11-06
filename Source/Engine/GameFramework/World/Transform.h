#pragma once
#include "Matrix4x4.hpp"
#include "Vector3.hpp"
#include "Quaternion.hpp"

// The sole public object transform. Actors are roots; spatial components store
// an actor-relative offset and compose it explicitly in SceneComponent.
class Transform
{
public:
	Transform() = default;
	Transform(const Transform&) = delete;
	Transform& operator=(const Transform&) = delete;
	Transform(Transform&&) = delete;
	Transform& operator=(Transform&&) = delete;

	//const TransformData& GetData() const { return myData; }
	//bool SetData(const TransformData& data);

	//Getters
	const CommonUtilities::Vector3f& GetLocalPosition() const { return myPosition; }
	CommonUtilities::Vector3f GetLocalRotationDegrees() const;
	CommonUtilities::Vector3f GetLocalRotationRadians() const;
	const CommonUtilities::Quaternion<float>& GetLocalRotation() const { return myRotation; };
	const CommonUtilities::Vector3f& GetLocalScale() const { return myScale; }

	const CommonUtilities::Matrix4f& GetLocalMatrix() const;
	const CommonUtilities::Matrix4f& GetWorldMatrix() const;
	const CommonUtilities::Vector3f& GetWorldPosition() const { return myPosition; }

	CommonUtilities::Vector3f GetLocalForward() const;
	CommonUtilities::Vector3f GetLocalRight() const;
	CommonUtilities::Vector3f GetLocalUp() const;

	//Setters
	void SetLocalPosition(const CommonUtilities::Vector3f& aPosition);
	void SetWorldPosition(const CommonUtilities::Vector3f& aPosition) { SetLocalPosition(aPosition); }
	void SetLocalRotationDegrees(float aPitch, float aYaw, float aRoll);
	void SetLocalRotationDegrees(const CommonUtilities::Vector3f& aRotation);
	//Takes radians
	void SetLocalRotation(const CommonUtilities::Vector3f& aRotation);

	void SetLocalRotation(const CommonUtilities::Quaternion<float>& aRotation);
	void SetLocalScale(const CommonUtilities::Vector3f& aScale);
	//For Unreal Matrix
	bool SetMatrix(const CommonUtilities::Matrix4x4<float>& aMatrix);

private:

	void RecalculateMatrix() const;

	CommonUtilities::Vector3f myPosition = { 0.0f, 0.0f, 0.0f };
	CommonUtilities::Quaternion<float> myRotation = CommonUtilities::Quaternion<float>::CreateIdentity();
	CommonUtilities::Vector3f myScale = { 1, 1, 1 };

	mutable CommonUtilities::Matrix4f myLocalMatrix;

	mutable bool myIsDirty = false;
};
