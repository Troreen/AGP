#pragma once
#include "Transform.hpp"

class World;
class Actor;
class SceneComponent;

// Parentless local TRS data. Copy this value to copy a pose, never an attachment.
struct LocalPose
{
    CommonUtilities::Vector3f Position{};
    CommonUtilities::Quaternion<float> Rotation{};
    CommonUtilities::Vector3f Scale{1, 1, 1};
};

// A stable object-owned transform. Setters reject invalid values without changing
// the pose. Parenting is available only on Actor and SceneComponent.
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
    const CommonUtilities::Vector3f& GetLocalPosition() const { return myValue.GetPosition(); }
    const CommonUtilities::Quaternion<float>& GetLocalRotation() const { return myValue.GetRotation(); }
    const CommonUtilities::Vector3f& GetLocalScale() const { return myValue.GetScale(); }
    bool SetLocalPosition(const CommonUtilities::Vector3f& position);
    bool SetLocalRotation(const CommonUtilities::Quaternion<float>& rotation);
    bool SetLocalRotationDegrees(float yaw, float pitch, float roll);
    bool SetLocalRotationRadians(float yaw, float pitch, float roll);
    bool SetLocalScale(const CommonUtilities::Vector3f& scale);
    CommonUtilities::Matrix4f GetLocalMatrix() const { return myValue.GetLocalMatrix(); }
    CommonUtilities::Matrix4f GetWorldMatrix() const { return myValue.GetWorldMatrix(); }
    CommonUtilities::Vector3f GetWorldPosition() const;
    bool SetWorldMatrix(const CommonUtilities::Matrix4f& matrix);
    bool SetWorldPosition(const CommonUtilities::Vector3f& position);

    // Temporary spelling compatibility for ModelViewer/controller migration.
    const CommonUtilities::Vector3f& GetPosition() const { return GetLocalPosition(); }
    const CommonUtilities::Quaternion<float>& GetRotation() const { return GetLocalRotation(); }
    const CommonUtilities::Vector3f& GetScale() const { return GetLocalScale(); }
    bool SetPosition(const CommonUtilities::Vector3f& value) { return SetLocalPosition(value); }
    bool SetRotation(const CommonUtilities::Quaternion<float>& value) { return SetLocalRotation(value); }
    bool SetRotation(float yaw, float pitch, float roll) { return SetLocalRotationDegrees(yaw, pitch, roll); }
    bool SetRotation(const CommonUtilities::Vector3f& value) { return SetLocalRotationDegrees(value.x, value.y, value.z); }
    bool SetScale(const CommonUtilities::Vector3f& value) { return SetLocalScale(value); }
    bool SetYawPitchRollRadians(float yaw, float pitch, float roll) { return SetLocalRotationRadians(yaw, pitch, roll); }
    CommonUtilities::Vector3f GetForward() const { return myValue.GetForward(); }
    CommonUtilities::Vector3f GetRight() const { return myValue.GetRight(); }
    CommonUtilities::Vector3f GetUp() const { return myValue.GetUp(); }

private:
    void EnsureMutationAllowed() const;
    CommonUtilities::Transform myValue;
    World* myWorld = nullptr;
    friend class World;
    friend class Actor;
    friend class SceneComponent;
};
