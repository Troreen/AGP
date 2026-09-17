#pragma once
#include "Transform.hpp"

// Parentless local TRS data. Copy this value to copy a pose, never an attachment.
struct LocalPose
{
	CommonUtilities::Vector3f Position{};
	CommonUtilities::Quaternion<float> Rotation{};
	CommonUtilities::Vector3f Scale{1, 1, 1};
};

// A stable object-owned transform. Setters reject invalid values without changing
// the pose. Actors have no hierarchy; spatial components may have a local offset.
class Transform
{
public:
	Transform() = default;
	Transform(const Transform&) = delete;
	Transform& operator=(const Transform&) = delete;
	Transform(Transform&&) = delete;
	Transform& operator=(Transform&&) = delete;

	LocalPose GetLocalPose() const;
	bool SetLocalPose(const LocalPose& pose);

	const CommonUtilities::Vector3f& GetLocalPosition() const
	{
		return myValue.GetPosition();
	}

	const CommonUtilities::Quaternion<float>& GetLocalRotation() const
	{
		return myValue.GetRotation();
	}

	const CommonUtilities::Vector3f& GetLocalScale() const
	{
		return myValue.GetScale();
	}

	bool SetLocalPosition(const CommonUtilities::Vector3f& position);
	bool SetLocalRotation(const CommonUtilities::Quaternion<float>& rotation);
	bool SetLocalRotationDegrees(float yaw, float pitch, float roll);
	bool SetLocalRotationRadians(float yaw, float pitch, float roll);
	bool SetLocalScale(const CommonUtilities::Vector3f& scale);

	CommonUtilities::Matrix4f GetLocalMatrix() const
	{
		return myValue.GetLocalMatrix();
	}

	CommonUtilities::Matrix4f GetWorldMatrix() const
	{
		return myValue.GetWorldMatrix();
	}

	CommonUtilities::Vector3f GetWorldPosition() const;

	bool SetWorldPosition(const CommonUtilities::Vector3f& position)
	{
		return SetLocalPosition(position);
	}

	CommonUtilities::Vector3f GetLocalForward() const
	{
		return myValue.GetForward();
	}

	CommonUtilities::Vector3f GetLocalRight() const
	{
		return myValue.GetRight();
	}

	CommonUtilities::Vector3f GetLocalUp() const
	{
		return myValue.GetUp();
	}

private:
	CommonUtilities::Transform myValue;
};
