#pragma once

#include <CommonUtilities/Matrix.hpp>
#include <GameFramework/Components/LightComponent.h>

class Camera
{
public:
    Camera();

    Camera(float aHorizontalFoV, float aNearPlane, float aFarPlane, float aCascadeSplits[DirectionalLightComponent::ShadowCascadeCount], float aAspectRatio);

    const CommonUtilities::Matrix4f& GetProjection() const { return myProjection; }
    const CommonUtilities::Matrix4f& GetProjectionByCascadeIndex(size_t aIndex) const;

private:
    CommonUtilities::Matrix4f CreatePerspectiveFovProjection(float aHorizontalFov, float aVerticalFov, float aNearPlane, float aFarPlane) const;

    float myHorizontalFoV;
    float myNearPlane;
    float myFarPlane;
    float myVerticalFoV;
	float myAspectRatio;

    CommonUtilities::Matrix4f myProjection;
    CommonUtilities::Matrix4f myNearProjection;
    CommonUtilities::Matrix4f myMidProjection;
    CommonUtilities::Matrix4f myFarProjection;
};

