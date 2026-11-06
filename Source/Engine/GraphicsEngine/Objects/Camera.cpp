#include "GraphicsEngine.pch.h"
#include "Camera.h"

#include <CommonUtilities/Utility.hpp>

Camera::Camera() :
	myProjection(CommonUtilities::Matrix4f::Identity),
	myNearPlane(0), myFarPlane(0),
	myHorizontalFoV(0), myVerticalFoV(0), myAspectRatio(0)
{
}

Camera::Camera(float aHorizontalFoV, float aNearPlane, float aFarPlane, float aCascadeSplits[DirectionalLightComponent::ShadowCascadeCount], float aAspectRatio)
{
    myNearPlane = aNearPlane;
    myFarPlane = aFarPlane;
    myHorizontalFoV = aHorizontalFoV;
    myAspectRatio = aAspectRatio;

    const float hFoVRad = aHorizontalFoV * CommonUtilities::Math::DEGREES_TO_RADIANS<float>;
    const float vFoVRad = 2 * std::atan(std::tan(hFoVRad * 0.5f) * aAspectRatio);

    myVerticalFoV = vFoVRad * CommonUtilities::Math::RADIANS_TO_DEGREES<float>;

    myProjection = CreatePerspectiveFovProjection(hFoVRad, vFoVRad, myNearPlane, myFarPlane);
	for (size_t i = 0; i < DirectionalLightComponent::ShadowCascadeCount; ++i)
	{
		myCascadeProjections[i] = CreatePerspectiveFovProjection(hFoVRad, vFoVRad, i == 0 ? myNearPlane : aCascadeSplits[i - 1], aCascadeSplits[i]);
	}
}

const CommonUtilities::Matrix4f& Camera::GetProjectionByCascadeIndex(size_t aIndex) const
{
	if (aIndex >= 0 && aIndex < DirectionalLightComponent::ShadowCascadeCount)
	{
		return myCascadeProjections[aIndex];
	}

    return myProjection;
}

CommonUtilities::Matrix4f Camera::CreatePerspectiveFovProjection(float aHorizontalFov, float aVerticalFov, float aNearPlane, float aFarPlane) const
{
    CommonUtilities::Matrix4f result;

    const float myXScale = 1 / std::tanf(aHorizontalFov * 0.5f);
    const float myYScale = 1 / std::tanf(aVerticalFov * 0.5f);
    const float zRange = aFarPlane / (aFarPlane - aNearPlane);

    result(1, 1) = myXScale;
    result(2, 2) = myYScale;
    result(3, 3) = zRange;
    result(3, 4) = 1.0f;
    result(4, 3) = -zRange * aNearPlane;
    result(4, 4) = 0.0f;

    return result;
}
