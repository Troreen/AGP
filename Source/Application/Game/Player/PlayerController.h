#pragma once
#include <GameFramework/Components/CustomComponent.h>
#include <Vector.hpp>

class PlayerMovementComponent;
class CameraComponent;

class PlayerController : public CustomComponent
{
public:
	PlayerController();

	void InitializeCustomVariables(const CustomComponentVariables& someVariables) override;
	void BeginPlay() override;
	void Update(float aDeltaTime) override;
	void SetScriptedControlLocked(bool aLocked);
	bool IsScriptedControlLocked() const;

private:

	PlayerMovementComponent* myMovement = nullptr;
	CameraComponent* myCamera = nullptr;
	bool myScriptedControlLocked = false;

	CommonUtilities::Vector3f myCameraOffset;

};
