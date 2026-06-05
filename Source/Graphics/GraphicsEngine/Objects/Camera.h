#pragma once

#include "Vector.hpp"
#include "Matrix.hpp"

using namespace CommonUtilities;

class Camera
{
public:
	Camera();
	Camera(float aHorizontalFoV, float aNearPlane, float aFarPlane, Vector2f aResolution);

	inline const Matrix4f& GetProjection() const { return myProjection; }

private:
	Matrix4f myProjection;

	float myNearPlane;
	float myFarPlane;
	float myHorizontalFoV;
	float myVerticalFoV;
};

