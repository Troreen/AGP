#pragma once

#include <cmath>

#include "Matrix3x3.hpp"
#include "Matrix4x4.hpp"
#include "Ray.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"

namespace CommonUtilities
{
    class Camera3D
    {
    public:
        enum class ProjectionType
        {
            Perspective,
            Orthographic
        };

        Camera3D() = default;

        template <typename ResolutionT>
        Camera3D(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const Vector2<ResolutionT>& aResolution)
        {
            const float width = static_cast<float>(aResolution.x) > 0.0f ? static_cast<float>(aResolution.x) : 1.0f;
            const float height = static_cast<float>(aResolution.y) > 0.0f ? static_cast<float>(aResolution.y) : 1.0f;
            const float horizontalFieldOfViewRadians = DegreesToRadians(aHorizontalFieldOfViewDegrees);
            const float verticalFieldOfViewRadians = 2.0f * std::atan(
                std::tan(horizontalFieldOfViewRadians * 0.5f) * (height / width));
            const float verticalFieldOfViewDegrees = RadiansToDegrees(verticalFieldOfViewRadians);
            const float aspectRatio = width / height;

            SetPerspective(verticalFieldOfViewDegrees, aspectRatio, aNearPlane, aFarPlane);
        }

        void SetPerspective(float aFieldOfViewDegrees, float aAspectRatio, float aNearPlane, float aFarPlane)
        {
            myProjectionType = ProjectionType::Perspective;
            myFieldOfViewRadians = DegreesToRadians(aFieldOfViewDegrees);
            myAspectRatio = aAspectRatio;
            myNearPlane = aNearPlane;
            myFarPlane = aFarPlane;
        }

        void SetOrthographic(float aWidth, float aHeight, float aNearPlane, float aFarPlane)
        {
            myProjectionType = ProjectionType::Orthographic;
            myOrthoLeft = -aWidth * 0.5f;
            myOrthoRight = aWidth * 0.5f;
            myOrthoBottom = -aHeight * 0.5f;
            myOrthoTop = aHeight * 0.5f;
            myNearPlane = aNearPlane;
            myFarPlane = aFarPlane;
        }

        void SetOrthographic(float aLeft, float aRight, float aBottom, float aTop, float aNearPlane, float aFarPlane)
        {
            myProjectionType = ProjectionType::Orthographic;
            myOrthoLeft = aLeft;
            myOrthoRight = aRight;
            myOrthoBottom = aBottom;
            myOrthoTop = aTop;
            myNearPlane = aNearPlane;
            myFarPlane = aFarPlane;
        }

        ProjectionType GetProjectionType() const { return myProjectionType; }

        float GetFieldOfView() const { return GetFieldOfViewDegrees(); }
        float GetFieldOfViewDegrees() const { return RadiansToDegrees(myFieldOfViewRadians); }
        float GetFieldOfViewRadians() const { return myFieldOfViewRadians; }
        float GetAspectRatio() const { return myAspectRatio; }
        float GetNearPlane() const { return myNearPlane; }
        float GetFarPlane() const { return myFarPlane; }

        void SetWorldMatrix(const Matrix4f& aWorldMatrix) { myWorldMatrix = aWorldMatrix; }
        const Matrix4f& GetWorldMatrix() const { return myWorldMatrix; }
        Vector3<float> GetPosition() const { return {myWorldMatrix(4, 1), myWorldMatrix(4, 2), myWorldMatrix(4, 3)}; }
        void SetPosition(const Vector3<float>& aPosition)
        {
            myWorldMatrix(4, 1) = aPosition.x;
            myWorldMatrix(4, 2) = aPosition.y;
            myWorldMatrix(4, 3) = aPosition.z;
        }
        void SetRotationDegrees(const Vector3<float>& aRotation)
        {
            const auto position = GetPosition();
            myWorldMatrix = Matrix4f::CreateRotationAroundY(DegreesToRadians(aRotation.x)) *
                Matrix4f::CreateRotationAroundX(DegreesToRadians(aRotation.y)) *
                Matrix4f::CreateRotationAroundZ(DegreesToRadians(aRotation.z));
            SetPosition(position);
        }

        Vector3<float> GetRight() const { return {myWorldMatrix(1, 1), myWorldMatrix(1, 2), myWorldMatrix(1, 3)}; }
        Vector3<float> GetUp() const { return {myWorldMatrix(2, 1), myWorldMatrix(2, 2), myWorldMatrix(2, 3)}; }
        Vector3<float> GetForward() const { return {myWorldMatrix(3, 1), myWorldMatrix(3, 2), myWorldMatrix(3, 3)}; }

        Matrix4f GetViewMatrix() const
        {
            return myWorldMatrix.GetFastInverse();
        }

        Matrix4f GetProjectionMatrix() const
        {
            Matrix4f projection;

            if (myProjectionType == ProjectionType::Perspective)
            {
                const float f = 1.0f / std::tan(myFieldOfViewRadians * 0.5f);
                projection(1, 1) = f / myAspectRatio;
                projection(2, 2) = f;
                projection(3, 3) = myFarPlane / (myFarPlane - myNearPlane);
                projection(3, 4) = 1.0f;
                projection(4, 3) = (-myNearPlane * myFarPlane) / (myFarPlane - myNearPlane);
                projection(4, 4) = 0.0f;
            }
            else
            {
                const float rightMinusLeft = myOrthoRight - myOrthoLeft;
                const float topMinusBottom = myOrthoTop - myOrthoBottom;
                const float farMinusNear = myFarPlane - myNearPlane;

                projection(1, 1) = 2.0f / rightMinusLeft;
                projection(2, 2) = 2.0f / topMinusBottom;
                projection(3, 3) = 1.0f / farMinusNear;
                projection(4, 1) = -(myOrthoRight + myOrthoLeft) / rightMinusLeft;
                projection(4, 2) = -(myOrthoTop + myOrthoBottom) / topMinusBottom;
                projection(4, 3) = -myNearPlane / farMinusNear;
            }

            return projection;
        }

        Matrix4f GetViewProjectionMatrix() const
        {
            return GetViewMatrix() * GetProjectionMatrix();
        }

        void LookAt(const Vector3<float>& aTarget)
        {
            const Vector3<float> position = GetPosition();
            Vector3<float> forward = (aTarget - position).GetNormalized();
            if (forward.LengthSqr() == 0.0f)
            {
                return;
            }

            const float yaw = std::atan2(forward.x, forward.z);
            const float pitch = -std::asin(forward.y);
            SetRotationDegrees({RadiansToDegrees(yaw), RadiansToDegrees(pitch), 0});
        }

        Ray<float> ScreenPointToRay(const Vector2<float>& aNormalizedScreenPos) const
        {
            const Vector3<float> origin = GetPosition();

            if (myProjectionType == ProjectionType::Orthographic)
            {
                const float ndcX = (aNormalizedScreenPos.x * 2.0f) - 1.0f;
                const float ndcY = 1.0f - (aNormalizedScreenPos.y * 2.0f);

                const Vector3<float> right = GetRight();
                const Vector3<float> up = GetUp();

                const Vector3<float> offset =
                    right * (ndcX * (myOrthoRight - myOrthoLeft) * 0.5f) +
                    up * (ndcY * (myOrthoTop - myOrthoBottom) * 0.5f);

                return Ray<float>(origin + offset, GetForward());
            }

            const float tanHalfFov = std::tan(myFieldOfViewRadians * 0.5f);
            const float ndcX = (aNormalizedScreenPos.x * 2.0f) - 1.0f;
            const float ndcY = 1.0f - (aNormalizedScreenPos.y * 2.0f);

            Vector3<float> localDir(
                ndcX * tanHalfFov * myAspectRatio,
                ndcY * tanHalfFov,
                1.0f);

            localDir = localDir.GetNormalized();

            Vector3<float> worldDir = GetRight() * localDir.x + GetUp() * localDir.y + GetForward() * localDir.z;
            worldDir.Normalize();

            return Ray<float>(origin, worldDir);
        }

        Vector3<float> WorldToScreenPoint(const Vector3<float>& aWorldPos) const
        {
            const Matrix4f viewProjection = GetViewProjectionMatrix();
            Vector4<float> clip = Vector4<float>(aWorldPos.x, aWorldPos.y, aWorldPos.z, 1.0f) * viewProjection;

            if (clip.w == 0.0f)
            {
                return Vector3<float>::Zero;
            }

            const float invW = 1.0f / clip.w;
            const Vector3<float> ndc(clip.x * invW, clip.y * invW, clip.z * invW);

            return Vector3<float>(
                (ndc.x + 1.0f) * 0.5f,
                (1.0f - ndc.y) * 0.5f,
                ndc.z);
        }

    private:
        static constexpr float DegreesToRadians(float value) { return value * 0.01745329251994329577f; }
        static constexpr float RadiansToDegrees(float value) { return value * 57.295779513082320876f; }
        Matrix4f myWorldMatrix;
        ProjectionType myProjectionType = ProjectionType::Perspective;

        float myFieldOfViewRadians = DegreesToRadians(90.0f);
        float myAspectRatio = 16.0f / 9.0f;
        float myNearPlane = 0.1f;
        float myFarPlane = 1000.0f;

        float myOrthoLeft = -1.0f;
        float myOrthoRight = 1.0f;
        float myOrthoBottom = -1.0f;
        float myOrthoTop = 1.0f;

    };
}
