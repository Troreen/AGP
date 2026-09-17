#pragma once

#include <cmath>

#include "Maths.hpp"
#include "Matrix3x3.hpp"
#include "Matrix4x4.hpp"
#include "Ray.hpp"
#include "Transform.hpp"
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
            const float horizontalFieldOfViewRadians = Maths::DegreesToRadians(aHorizontalFieldOfViewDegrees);
            const float verticalFieldOfViewRadians = 2.0f * std::atan(
                std::tan(horizontalFieldOfViewRadians * 0.5f) * (height / width));
            const float verticalFieldOfViewDegrees = Maths::RadiansToDegrees(verticalFieldOfViewRadians);
            const float aspectRatio = width / height;

            SetPerspective(verticalFieldOfViewDegrees, aspectRatio, aNearPlane, aFarPlane);
        }

        void SetPerspective(float aFieldOfViewDegrees, float aAspectRatio, float aNearPlane, float aFarPlane)
        {
            myProjectionType = ProjectionType::Perspective;
            myFieldOfViewRadians = Maths::DegreesToRadians(aFieldOfViewDegrees);
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
        float GetFieldOfViewDegrees() const { return Maths::RadiansToDegrees(myFieldOfViewRadians); }
        float GetFieldOfViewRadians() const { return myFieldOfViewRadians; }
        float GetAspectRatio() const { return myAspectRatio; }
        float GetNearPlane() const { return myNearPlane; }
        float GetFarPlane() const { return myFarPlane; }

        Transform& GetTransform() { return myTransform; }
        const Transform& GetTransform() const { return myTransform; }

        Vector3<float> GetRight() const { return myTransform.GetRight(); }
        Vector3<float> GetUp() const { return myTransform.GetUp(); }
        Vector3<float> GetForward() const { return myTransform.GetForward(); }

        Matrix4f GetViewMatrix() const
        {
            return myTransform.GetWorldMatrix().GetFastInverse();
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

        void LookAt(const Vector3<float>& aTarget, const Vector3<float>& aUp = Vector3<float>::UnitY)
        {
            const Vector3<float> position = myTransform.GetPosition();
            Vector3<float> forward = (aTarget - position).GetNormalized();
            if (forward.LengthSqr() == 0.0f)
            {
                return;
            }

            const float yaw = std::atan2(forward.x, forward.z);
            const float pitch = -std::asin(forward.y);
            myTransform.SetYawPitchRollRadians(yaw, pitch, 0.0f);
        }

        Ray<float> ScreenPointToRay(const Vector2<float>& aNormalizedScreenPos) const
        {
            const Vector3<float> origin = myTransform.GetPosition();

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

            const Matrix3x3<float> rotation = myTransform.GetRotation().ToMatrix3x3();
            Vector3<float> worldDir = localDir * rotation;
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

        void SetFollowTarget(Transform* aTarget, const Vector3<float>& anOffset = Vector3<float>::Zero,
            bool aUseTargetRotation = false, bool aLookAtTarget = true)
        {
            myFollowTarget = aTarget;
            myFollowOffset = anOffset;
            myFollowUseTargetRotation = aUseTargetRotation;
            myFollowLookAtTarget = aLookAtTarget;
        }

        void ClearFollowTarget()
        {
            myFollowTarget = nullptr;
        }

        bool HasFollowTarget() const { return myFollowTarget != nullptr; }

        void UpdateFollow()
        {
            if (!myFollowTarget)
            {
                return;
            }

            Vector3<float> offset = myFollowOffset;
            if (myFollowUseTargetRotation)
            {
                const Matrix3x3<float> rotation = myFollowTarget->GetRotation().ToMatrix3x3();
                offset = offset * rotation;
            }

            const Vector3<float> targetPos = myFollowTarget->GetPosition();
            myTransform.SetPosition(targetPos + offset);

            if (myFollowLookAtTarget)
            {
                LookAt(targetPos);
            }
        }

    private:
        Transform myTransform;
        ProjectionType myProjectionType = ProjectionType::Perspective;

        float myFieldOfViewRadians = Maths::DegreesToRadians(90.0f);
        float myAspectRatio = 16.0f / 9.0f;
        float myNearPlane = 0.1f;
        float myFarPlane = 1000.0f;

        float myOrthoLeft = -1.0f;
        float myOrthoRight = 1.0f;
        float myOrthoBottom = -1.0f;
        float myOrthoTop = 1.0f;

        Transform* myFollowTarget = nullptr;
        Vector3<float> myFollowOffset = Vector3<float>::Zero;
        bool myFollowUseTargetRotation = false;
        bool myFollowLookAtTarget = true;
    };
}
