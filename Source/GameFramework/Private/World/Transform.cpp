#include "GameFramework/Transform.h"
#include "World/TransformOperations.h"
#include "GameFramework/World.h"
#include <cmath>

namespace
{
    bool Finite(const CommonUtilities::Vector3f& value)
    {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    bool Normalize(const CommonUtilities::Quaternion<float>& value, CommonUtilities::Quaternion<float>& result)
    {
        if (!std::isfinite(value.w) || !std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) return false;
        const double length = std::sqrt(double(value.w) * value.w + double(value.x) * value.x
            + double(value.y) * value.y + double(value.z) * value.z);
        if (length == 0) return false;
        result = {float(value.w / length), float(value.x / length), float(value.y / length), float(value.z / length)};
        return true;
    }
}

void Transform::EnsureMutationAllowed() const
{
    if (myWorld) myWorld->EnsureComponentMutationAllowed(myComponent);
}

LocalPose Transform::GetLocalPose() const
{
    return {myValue.GetPosition(), myValue.GetRotation(), myValue.GetScale()};
}

bool Transform::SetLocalPose(const LocalPose& pose)
{
    EnsureMutationAllowed();
    CommonUtilities::Quaternion<float> rotation;
    if (!Finite(pose.Position) || !Finite(pose.Scale) || !Normalize(pose.Rotation, rotation)) return false;
    const CommonUtilities::Transform candidate(pose.Position, rotation, pose.Scale);
    const auto& matrix = candidate.GetLocalMatrix();
    for (int row = 1; row <= 4; ++row)
        for (int column = 1; column <= 4; ++column)
            if (!std::isfinite(matrix(row, column))) return false;
    myValue.SetPosition(pose.Position);
    myValue.SetRotation(rotation);
    myValue.SetScale(pose.Scale);
    return true;
}

bool Transform::SetLocalPosition(const CommonUtilities::Vector3f& position)
{
    auto pose = GetLocalPose(); pose.Position = position;
    return SetLocalPose(pose);
}

bool Transform::SetLocalRotation(const CommonUtilities::Quaternion<float>& rotation)
{
    auto pose = GetLocalPose(); pose.Rotation = rotation;
    return SetLocalPose(pose);
}

bool Transform::SetLocalRotationDegrees(float yaw, float pitch, float roll)
{
    return SetLocalRotationRadians(CommonUtilities::Maths::DegreesToRadians(yaw),
        CommonUtilities::Maths::DegreesToRadians(pitch), CommonUtilities::Maths::DegreesToRadians(roll));
}

bool Transform::SetLocalRotationRadians(float yaw, float pitch, float roll)
{
    EnsureMutationAllowed();
    if (!std::isfinite(yaw) || !std::isfinite(pitch) || !std::isfinite(roll)) return false;
    return SetLocalRotation(CommonUtilities::Quaternion<float>::CreateFromYawPitchRoll(yaw, pitch, roll));
}

bool Transform::SetLocalScale(const CommonUtilities::Vector3f& scale)
{
    auto pose = GetLocalPose(); pose.Scale = scale;
    return SetLocalPose(pose);
}

CommonUtilities::Vector3f Transform::GetWorldPosition() const
{
    const auto matrix = GetWorldMatrix();
    return {matrix(4, 1), matrix(4, 2), matrix(4, 3)};
}

bool Transform::SetWorldMatrix(const CommonUtilities::Matrix4f& matrix)
{
    EnsureMutationAllowed();
    return GameFrameworkInternal::SetWorldMatrix(myValue, matrix);
}

bool Transform::SetWorldPosition(const CommonUtilities::Vector3f& position)
{
    EnsureMutationAllowed();
    if (!Finite(position)) return false;
    auto matrix = GetWorldMatrix();
    matrix(4, 1) = position.x; matrix(4, 2) = position.y; matrix(4, 3) = position.z;
    return SetWorldMatrix(matrix);
}
