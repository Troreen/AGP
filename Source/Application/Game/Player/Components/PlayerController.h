#pragma once

#include <GameFramework/Helper/Event.hpp>
#include <GameFramework/Components/CustomComponent.h>
#include <Vector.hpp>

namespace CommonUtilities
{
	struct InputEvent;
}

class PlayerMovementComponent;
class CameraComponent;
class AbilityComponent;

class PlayerController : public CustomComponent
{
public:
	PlayerController();
	~PlayerController() override;

	void InitializeCustomVariables(const CustomComponentVariables& someVariables) override;
	void BeginPlay() override;
	void EndPlay() noexcept override;
	void Update(float aDeltaTime) override;
	void SetScriptedControlLocked(bool aLocked);
	bool IsScriptedControlLocked() const;

private:
	void NavigationHandler(const CommonUtilities::InputEvent& aEvent);

	PlayerMovementComponent* myMovement = nullptr;
	AbilityComponent* myAbility = nullptr;
	CameraComponent* myCamera = nullptr;
	bool myScriptedControlLocked = false;

	unsigned myNavigationEventId = 0;
	std::vector<std::tuple<std::string, unsigned, ListenerIdentification>> myAnimationEventIds{};

	CommonUtilities::Vector3f myCameraOffset;
};
