#include "GraphicsEngine.pch.h"
#include "Camera.h"
#include "Maths.hpp"

Camera::Camera()
	: myProjection(Matrix4f()), myNearPlane (0), myFarPlane(0), myHorizontalFoV(0), myVerticalFoV(0)
{
} 

Camera::Camera(float aHorizontalFoV, float aNearPlane, float aFarPlane, Vector2f aResolution)
{
	myNearPlane = aNearPlane;
	myFarPlane = aFarPlane;
	myHorizontalFoV = aHorizontalFoV;	

	const float hFoVRad = Maths::DegreesToRadians(aHorizontalFoV);

	const float vFoVRad = 2 * std::atan(std::tan(hFoVRad * 0.5f) * (static_cast<float>(aResolution.y) / static_cast<float>(aResolution.x)));
	
	myVerticalFoV = Maths::RadiansToDegrees(vFoVRad);

	const float myXScale = 1 / std::tan(hFoVRad * 0.5f);
	const float myYScale = 1 / std::tan(vFoVRad * 0.5f);
	const float zRange = myFarPlane / (myFarPlane - myNearPlane);

	myProjection(1, 1) = myXScale;
	myProjection(2, 2) = myYScale;
	myProjection(3, 3) = zRange;
	myProjection(3, 4) = 1.0f;
	myProjection(4, 3) = -zRange * myNearPlane;
	myProjection(4, 4) = 0.0f;
}
