#pragma once
#include "GameFramework/Components/CustomComponent.h"

#include <string>

class Transform;
struct CustomComponentVariables;

// Continuously rotates the owning Actor or a named SceneComponent.
class SpinComponent final : public CustomComponent
{
public:
	void SetTargetComponentName(const char* aComponentName);
	void SetDegreesPerSecond(float aDegreesPerSecond);
	void ApplyCustomVariables(const CustomComponentVariables& someVariables) override;
	void BeginPlay() override;
	void Update(float deltaTime) override;

private:
	Transform* FindTargetTransform() const;
	std::string myTargetComponentName;
	float myYaw = 0;
	float myDegreesPerSecond = 25.0f;
};
