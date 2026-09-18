#pragma once
#include "Matrix4x4.hpp"
#include "Vector3.hpp"

// Copyable scene/authored transform data. Rotation is yaw (Y), pitch (X),
// roll (Z), in degrees. Runtime Transform objects remain stable and noncopyable.
struct TransformData
{
	CommonUtilities::Vector3f Position{};
	CommonUtilities::Vector3f RotationDegrees{};
	CommonUtilities::Vector3f Scale{1, 1, 1};
};

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

	TransformData GetData() const { return myData; }
	bool SetData(const TransformData& data);

	const CommonUtilities::Vector3f& GetLocalPosition() const { return myData.Position; }
	const CommonUtilities::Vector3f& GetLocalRotationDegrees() const { return myData.RotationDegrees; }
	const CommonUtilities::Vector3f& GetLocalScale() const { return myData.Scale; }

	bool SetLocalPosition(const CommonUtilities::Vector3f& position);
	bool SetLocalRotationDegrees(float yaw, float pitch, float roll);
	bool SetLocalRotationDegrees(const CommonUtilities::Vector3f& rotation);
	bool SetLocalScale(const CommonUtilities::Vector3f& scale);

	const CommonUtilities::Matrix4f& GetLocalMatrix() const { return myLocalMatrix; }
	const CommonUtilities::Matrix4f& GetWorldMatrix() const { return myLocalMatrix; }
	CommonUtilities::Vector3f GetWorldPosition() const { return myData.Position; }
	bool SetWorldPosition(const CommonUtilities::Vector3f& position) { return SetLocalPosition(position); }

	CommonUtilities::Vector3f GetLocalForward() const;
	CommonUtilities::Vector3f GetLocalRight() const;
	CommonUtilities::Vector3f GetLocalUp() const;

private:
	TransformData myData;
	CommonUtilities::Matrix4f myLocalMatrix;
};
