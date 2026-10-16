#pragma once

#include "SceneComponent.h"
#include "LightComponent.h"

#include <array>
#include <CommonUtilities/CommonUtilities.h>
#include <CommonUtilities/Ray.hpp>
#include <GraphicsEngine/Objects/Camera.h>

// Projection/view data attached to an Actor. This component does not
// implement player controls. Select the specific CameraComponent through
// World::SetActiveCamera; the engine handles camera extraction.
class CameraComponent final : public SceneComponent
{
public:
	static constexpr float DefaultCascadeSplits[DirectionalLightComponent::ShadowCascadeCount] = { 4000, 8000, 20000 };
	static constexpr float DefaultFieldOfView = 90.0f;
	static constexpr float DefaultNearPlane = 1.0f;
	static constexpr float DefaultFarPlane = 50000.0f;
	static constexpr float DefaultAspectRatio = 16.0f / 9.0f;

	CameraComponent();
	CameraComponent(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const CommonUtilities::Vector2u& aResolution);

	// Invalid projection values are rejected without changing the camera.
	bool SetPerspective(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane,
	                    const CommonUtilities::Vector2u& aResolution);
	// Rebuild the projection with the camera's existing field of view and clipping planes.
	bool SetResolution(const CommonUtilities::Vector2u& aResolution);
	const Camera& GetCamera() const;

	float GetFieldOfView() const { return GetFieldOfViewDegrees(); }
	float GetFieldOfViewDegrees() const { return myHorizontalFieldOfViewRadians * CU::Math::RADIANS_TO_DEGREES<float>; }
	float GetFieldOfViewRadians() const { return myHorizontalFieldOfViewRadians; }
	float GetNearPlane() const { return myNearPlane; }
	float GetFarPlane() const { return myFarPlane; }
	float GetAspectRatio() const { return myAspectRatio; }
	//bool GetIsDebug() const { return };

	CU::Matrix4f GetView() const;
	const CU::Matrix4f& GetProjection() const;
	const CU::Matrix4f& GetProjectionByCascadeIndex(size_t aIndex) const;
	float GetCascadeSplit(size_t aIndex) const;

	std::array<CU::Vector3f, 8> GetFrustumCorners() const;
	std::array<CU::Vector3f, 8> GetFrustumCorners(float aNearPlane, float aFarPlane) const;

	void LookAt(const CU::Vector3<float>& aTarget);
	CU::Ray<float> ScreenPointToRay(const CU::Vector2<float>& aScreenPos, const CU::Vector2<unsigned>& aClientSize) const;
	CU::Vector3<float> WorldToScreenPoint(const CU::Vector3<float>& aWorldPos) const;

private:
	Camera myCamera;

	float myCascadeSplits[DirectionalLightComponent::ShadowCascadeCount];

	float myHorizontalFieldOfViewRadians = DefaultFieldOfView * CU::Math::DEGREES_TO_RADIANS<float>;
	float myNearPlane = DefaultNearPlane;
	float myFarPlane = DefaultFarPlane;
	float myAspectRatio = DefaultAspectRatio;
};
