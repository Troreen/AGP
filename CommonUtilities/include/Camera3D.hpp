#pragma once

#include <cmath>

#include "Matrix4x4.hpp"
#include "Quaternion.hpp"
#include "Ray.hpp"
#include "Transform.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"

namespace CommonUtilities
{
    template <typename T>
    class Camera3D
    {
    public:
        enum class ProjectionType
        {
            Perspective,
            Orthographic
        };

        Camera3D() = default;

        void SetPerspective(const T aFieldOfViewRadians, const T aAspectRatio, const T aNearPlane, const T aFarPlane)
        {
            myProjectionType = ProjectionType::Perspective;
            myFieldOfView = aFieldOfViewRadians;
            myAspectRatio = aAspectRatio;
            myNearPlane = aNearPlane;
            myFarPlane = aFarPlane;
        }

        void SetOrthographic(const T aWidth, const T aHeight, const T aNearPlane, const T aFarPlane)
        {
            myProjectionType = ProjectionType::Orthographic;
            myOrthoLeft = -aWidth * static_cast<T>(0.5);
            myOrthoRight = aWidth * static_cast<T>(0.5);
            myOrthoBottom = -aHeight * static_cast<T>(0.5);
            myOrthoTop = aHeight * static_cast<T>(0.5);
            myNearPlane = aNearPlane;
            myFarPlane = aFarPlane;
        }

        void SetOrthographic(const T aLeft, const T aRight, const T aBottom, const T aTop, const T aNearPlane, const T aFarPlane)
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

        T GetFieldOfView() const { return myFieldOfView; }
        T GetAspectRatio() const { return myAspectRatio; }
        T GetNearPlane() const { return myNearPlane; }
        T GetFarPlane() const { return myFarPlane; }

        Transform<T>& GetTransform() { return myTransform; }
        const Transform<T>& GetTransform() const { return myTransform; }

        Vector3<T> GetRight() const { return myTransform.GetRight(); }
        Vector3<T> GetUp() const { return myTransform.GetUp(); }
        Vector3<T> GetForward() const { return myTransform.GetForward(); }

        Matrix4x4<T> GetViewMatrix()
        {
            return myTransform.GetWorldMatrix().GetFastInverse();
        }

        Matrix4x4<T> GetProjectionMatrix() const
        {
            Matrix4x4<T> projection;

            if (myProjectionType == ProjectionType::Perspective)
            {
                const T f = static_cast<T>(1) / static_cast<T>(std::tan(myFieldOfView * static_cast<T>(0.5)));
                projection(1, 1) = f / myAspectRatio;
                projection(2, 2) = f;
                projection(3, 3) = myFarPlane / (myFarPlane - myNearPlane);
                projection(3, 4) = static_cast<T>(1);
                projection(4, 3) = (-myNearPlane * myFarPlane) / (myFarPlane - myNearPlane);
                projection(4, 4) = static_cast<T>(0);
            }
            else
            {
                const T rightMinusLeft = myOrthoRight - myOrthoLeft;
                const T topMinusBottom = myOrthoTop - myOrthoBottom;
                const T farMinusNear = myFarPlane - myNearPlane;

                projection(1, 1) = static_cast<T>(2) / rightMinusLeft;
                projection(2, 2) = static_cast<T>(2) / topMinusBottom;
                projection(3, 3) = static_cast<T>(1) / farMinusNear;
                projection(4, 1) = -(myOrthoRight + myOrthoLeft) / rightMinusLeft;
                projection(4, 2) = -(myOrthoTop + myOrthoBottom) / topMinusBottom;
                projection(4, 3) = -myNearPlane / farMinusNear;
            }

            return projection;
        }

        Matrix4x4<T> GetViewProjectionMatrix()
        {
            return GetViewMatrix() * GetProjectionMatrix();
        }

        void LookAt(const Vector3<T>& aTarget, const Vector3<T>& aUp = Vector3<T>::UnitY)
        {
            const Vector3<T> position = myTransform.GetPosition();
            Vector3<T> forward = (aTarget - position).GetNormalized();
            if (forward.LengthSqr() == static_cast<T>(0))
            {
                return;
            }

            const T yaw = static_cast<T>(std::atan2(forward.x, forward.z));
            const T pitch = static_cast<T>(-std::asin(forward.y));
            myTransform.SetYawPitchRollRadians(yaw, pitch, static_cast<T>(0));
        }

        Ray<T> ScreenPointToRay(const Vector2<T>& aNormalizedScreenPos) const
        {
            const Vector3<T> origin = myTransform.GetPosition();

            if (myProjectionType == ProjectionType::Orthographic)
            {
                const T ndcX = (aNormalizedScreenPos.x * static_cast<T>(2)) - static_cast<T>(1);
                const T ndcY = static_cast<T>(1) - (aNormalizedScreenPos.y * static_cast<T>(2));

                const Vector3<T> right = GetRight();
                const Vector3<T> up = GetUp();

                const Vector3<T> offset =
                    right * (ndcX * (myOrthoRight - myOrthoLeft) * static_cast<T>(0.5)) +
                    up * (ndcY * (myOrthoTop - myOrthoBottom) * static_cast<T>(0.5));

                return Ray<T>(origin + offset, GetForward());
            }

            const T tanHalfFov = static_cast<T>(std::tan(myFieldOfView * static_cast<T>(0.5)));
            const T ndcX = (aNormalizedScreenPos.x * static_cast<T>(2)) - static_cast<T>(1);
            const T ndcY = static_cast<T>(1) - (aNormalizedScreenPos.y * static_cast<T>(2));

            Vector3<T> localDir(
                ndcX * tanHalfFov * myAspectRatio,
                ndcY * tanHalfFov,
                static_cast<T>(1));

            localDir = localDir.GetNormalized();

            const Matrix3x3<T> rotation = myTransform.GetRotation().ToMatrix3x3();
            Vector3<T> worldDir = localDir * rotation;
            worldDir.Normalize();

            return Ray<T>(origin, worldDir);
        }

        Vector3<T> WorldToScreenPoint(const Vector3<T>& aWorldPos)
        {
            const Matrix4x4<T> viewProjection = GetViewProjectionMatrix();
            Vector4<T> clip = Vector4<T>(aWorldPos.x, aWorldPos.y, aWorldPos.z, static_cast<T>(1)) * viewProjection;

            if (clip.w == static_cast<T>(0))
            {
                return Vector3<T>::Zero;
            }

            const T invW = static_cast<T>(1) / clip.w;
            const Vector3<T> ndc(clip.x * invW, clip.y * invW, clip.z * invW);

            return Vector3<T>(
                (ndc.x + static_cast<T>(1)) * static_cast<T>(0.5),
                (static_cast<T>(1) - ndc.y) * static_cast<T>(0.5),
                ndc.z);
        }

        void SetFollowTarget(Transform<T>* aTarget, const Vector3<T>& anOffset = Vector3<T>::Zero,
            const bool aUseTargetRotation = false, const bool aLookAtTarget = true)
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

            Vector3<T> offset = myFollowOffset;
            if (myFollowUseTargetRotation)
            {
                const Matrix3x3<T> rotation = myFollowTarget->GetRotation().ToMatrix3x3();
                offset = offset * rotation;
            }

            const Vector3<T> targetPos = myFollowTarget->GetPosition();
            myTransform.SetPosition(targetPos + offset);

            if (myFollowLookAtTarget)
            {
                LookAt(targetPos);
            }
        }

    private:
        Transform<T> myTransform;
        ProjectionType myProjectionType = ProjectionType::Perspective;

        T myFieldOfView = static_cast<T>(1.57079632679); // 90 degrees in radians
        T myAspectRatio = static_cast<T>(16) / static_cast<T>(9);
        T myNearPlane = static_cast<T>(0.1);
        T myFarPlane = static_cast<T>(1000.0);

        T myOrthoLeft = static_cast<T>(-1);
        T myOrthoRight = static_cast<T>(1);
        T myOrthoBottom = static_cast<T>(-1);
        T myOrthoTop = static_cast<T>(1);

        Transform<T>* myFollowTarget = nullptr;
        Vector3<T> myFollowOffset = Vector3<T>::Zero;
        bool myFollowUseTargetRotation = false;
        bool myFollowLookAtTarget = true;
    };

    using Camera3Df = Camera3D<float>;
}
