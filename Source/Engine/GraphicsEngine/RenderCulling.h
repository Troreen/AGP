#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include "Camera3D.hpp"
#include "Matrix.hpp"

namespace RenderCulling
{
	inline bool IsFinite(const CU::Vector3f& v)
	{
		return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
	}

	struct BoundingSphere
	{
		CU::Vector3f Center = CU::Vector3f::Zero;
		float Radius = 0.0f;
		bool IsValid = false;
	};

	struct FrustumPlane
	{
		CU::Vector3f Normal = CU::Vector3f::Zero;
		float D = 0.0f;
		bool IsValid = false;
	};

	struct CameraFrustum
	{
		std::array<FrustumPlane, 6> Planes = {};
		bool IsValid = false;
	};

	inline FrustumPlane CreateFrustumPlane(const CU::Vector4f& aPlane)
	{
		FrustumPlane plane;
		plane.Normal = {aPlane.x, aPlane.y, aPlane.z};
		const float normalLength = plane.Normal.Length();
		if (!std::isfinite(normalLength) || !std::isfinite(aPlane.w) || normalLength <= 0.000001f)
		{
			return {};
		}

		const float invNormalLength = 1.0f / normalLength;
		plane.Normal *= invNormalLength;
		plane.D = aPlane.w * invNormalLength;
		plane.IsValid = true;
		return plane;
	}

	inline float GetSignedDistance(const FrustumPlane& aPlane, const CU::Vector3f& aPosition)
	{
		return aPlane.Normal.Dot(aPosition) + aPlane.D;
	}

	inline CameraFrustum CreateFrustumFromViewProjection(const CU::Matrix4f& aViewProjection)
	{
		const CU::Vector4f x = aViewProjection.GetColumn(1);
		const CU::Vector4f y = aViewProjection.GetColumn(2);
		const CU::Vector4f z = aViewProjection.GetColumn(3);
		const CU::Vector4f w = aViewProjection.GetColumn(4);

		CameraFrustum frustum;
		frustum.Planes[0] = CreateFrustumPlane(x + w);
		frustum.Planes[1] = CreateFrustumPlane(-x + w);
		frustum.Planes[2] = CreateFrustumPlane(y + w);
		frustum.Planes[3] = CreateFrustumPlane(-y + w);
		frustum.Planes[4] = CreateFrustumPlane(z);
		frustum.Planes[5] = CreateFrustumPlane(-z + w);
		frustum.IsValid = std::ranges::all_of(frustum.Planes, [](const FrustumPlane& aPlane)
		{
			return aPlane.IsValid;
		});
		return frustum;
	}

	inline CameraFrustum CreateCameraFrustum(const CU::Camera3D& aCamera)
	{
		return CreateFrustumFromViewProjection(aCamera.GetViewProjectionMatrix());
	}

	inline bool IntersectsFrustum(const CameraFrustum& aFrustum, const BoundingSphere& aSphere)
	{
		if (!aFrustum.IsValid)
		{
			return true;
		}

		if (!aSphere.IsValid || !IsFinite(aSphere.Center) || !std::isfinite(aSphere.Radius) || aSphere.Radius < 0.0f)
		{
			return true;
		}

		for (const FrustumPlane& plane : aFrustum.Planes)
		{
			if (GetSignedDistance(plane, aSphere.Center) < -aSphere.Radius)
			{
				return false;
			}
		}

		return true;
	}

	inline float GetMaxAxisScale(const CU::Matrix4f& aTransform)
	{
		const CU::Vector3f axisX(aTransform(1, 1), aTransform(1, 2), aTransform(1, 3));
		const CU::Vector3f axisY(aTransform(2, 1), aTransform(2, 2), aTransform(2, 3));
		const CU::Vector3f axisZ(aTransform(3, 1), aTransform(3, 2), aTransform(3, 3));
		// Gershgorin's bound on the largest eigenvalue of A*A^T is exact for
		// orthogonal axes and conservative for shear (including parent transforms).
		const float xy = std::abs(axisX.Dot(axisY));
		const float xz = std::abs(axisX.Dot(axisZ));
		const float yz = std::abs(axisY.Dot(axisZ));
		return std::sqrt((std::max)({axisX.LengthSqr() + xy + xz, axisY.LengthSqr() + xy + yz, axisZ.LengthSqr() + xz + yz}));
	}

	inline BoundingSphere TransformBoundingSphere(const CU::Vector3f& aCenter, float aRadius, bool aIsValid, const CU::Matrix4f& aTransform)
	{
		for (int row = 1; row <= 4; ++row)
		{
			for (int col = 1; col <= 4; ++col)
			{
				if (!std::isfinite(aTransform(row, col)))
				{
					return {};
				}
			}
		}
		if (!aIsValid || !IsFinite(aCenter) || !std::isfinite(aRadius) || aRadius < 0.0f || aTransform(1, 4) != 0.0f ||
		    aTransform(2, 4) != 0.0f || aTransform(3, 4) != 0.0f || aTransform(4, 4) != 1.0f)
		{
			return {};
		}

		BoundingSphere sphere;
		sphere.Center = CU::Maths::TransformPoint(aCenter, aTransform);
		sphere.Radius = aRadius * GetMaxAxisScale(aTransform);
		sphere.IsValid = IsFinite(sphere.Center) && sphere.Radius >= 0.0f && std::isfinite(sphere.Radius);
		return sphere;
	}

}
