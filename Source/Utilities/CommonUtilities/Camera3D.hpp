#pragma once

#include <cmath>

#include "Matrix4x4.hpp"
#include "Ray.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Utility.hpp"

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
        Camera3D(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const Vector2<ResolutionT>& aResolution);

        void SetPerspective(float aFieldOfViewDegrees, float aAspectRatio, float aNearPlane, float aFarPlane);
        void SetOrthographic(float aWidth, float aHeight, float aNearPlane, float aFarPlane);
        void SetOrthographic(float aLeft, float aRight, float aBottom, float aTop, float aNearPlane, float aFarPlane);

        ProjectionType GetProjectionType() const { return myProjectionType; }

        float GetFieldOfView() const { return GetFieldOfViewDegrees(); }
        float GetFieldOfViewDegrees() const { return myFieldOfViewRadians * Math::RADIANS_TO_DEGREES<float>; }
        float GetFieldOfViewRadians() const { return myFieldOfViewRadians; }
        float GetAspectRatio() const { return myAspectRatio; }
        float GetNearPlane() const { return myNearPlane; }
        float GetFarPlane() const { return myFarPlane; }

        void SetWorldMatrix(const Matrix4f& aWorldMatrix) { myWorldMatrix = aWorldMatrix; }
        const Matrix4f& GetWorldMatrix() const { return myWorldMatrix; }
        Vector3<float> GetPosition() const { return {myWorldMatrix(4, 1), myWorldMatrix(4, 2), myWorldMatrix(4, 3)}; }
        void SetPosition(const Vector3<float>& aPosition);
        void SetRotationDegrees(const Vector3<float>& aRotation);

        Vector3<float> GetRight() const { return {myWorldMatrix(1, 1), myWorldMatrix(1, 2), myWorldMatrix(1, 3)}; }
        Vector3<float> GetUp() const { return {myWorldMatrix(2, 1), myWorldMatrix(2, 2), myWorldMatrix(2, 3)}; }
        Vector3<float> GetForward() const { return {myWorldMatrix(3, 1), myWorldMatrix(3, 2), myWorldMatrix(3, 3)}; }

        Matrix4f GetViewMatrix() const { return myWorldMatrix.GetFastInverse(); }
        Matrix4f GetProjectionMatrix() const;
        Matrix4f GetViewProjectionMatrix() const { return GetViewMatrix() * GetProjectionMatrix(); }

		std::array<Vector3f, 8> GetFrustumCorners() const;
		std::array<Vector3f, 8> GetFrustumCorners(float aNearPlane, float aFarPlane) const;

        void LookAt(const Vector3<float>& aTarget);
        Ray<float> ScreenPointToRay(const Vector2<float>& aNormalizedScreenPos) const;
        Vector3<float> WorldToScreenPoint(const Vector3<float>& aWorldPos) const;

    private:
        Matrix4f myWorldMatrix;
        ProjectionType myProjectionType = ProjectionType::Perspective;

        float myFieldOfViewRadians = 90.0f * Math::DEGREES_TO_RADIANS<float>;
        float myAspectRatio = 16.0f / 9.0f;
        float myNearPlane = 0.1f;
        float myFarPlane = 1000.0f;

        float myOrthoLeft = -1.0f;
        float myOrthoRight = 1.0f;
        float myOrthoBottom = -1.0f;
        float myOrthoTop = 1.0f;

    };

	template<typename ResolutionT>
	inline Camera3D::Camera3D(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const Vector2<ResolutionT>& aResolution)
	{
		const float width = static_cast<float>(aResolution.x) > 0.0f ? static_cast<float>(aResolution.x) : 1.0f;
		const float height = static_cast<float>(aResolution.y) > 0.0f ? static_cast<float>(aResolution.y) : 1.0f;

		const float horizontalFieldOfViewRadians = aHorizontalFieldOfViewDegrees * Math::DEGREES_TO_RADIANS<float>;
		const float verticalFieldOfViewRadians = 2.0f * std::atan(std::tan(horizontalFieldOfViewRadians * 0.5f) * (height / width));
		const float verticalFieldOfViewDegrees = verticalFieldOfViewRadians * Math::RADIANS_TO_DEGREES<float>;

		const float aspectRatio = width / height;

		SetPerspective(verticalFieldOfViewDegrees, aspectRatio, aNearPlane, aFarPlane);
	}
}
