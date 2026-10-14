#pragma once
#include <GameFramework\World\Component.h>
#include <Vector.hpp>

class PlayerMovementComponent;
class CameraComponent;

class PlayerController : public Component
{
public:
	PlayerController();

	void BeginPlay() override;
	void Update(float aDeltaTime) override;

private:

	PlayerMovementComponent* myMovement = nullptr;
	CameraComponent* myCamera = nullptr;

	CommonUtilities::Vector3f myCameraOffset;

};
