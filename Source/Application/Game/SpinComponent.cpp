#include "SpinComponent.h"

#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/World/Actor.h"

#include <cmath>

namespace
{
	constexpr float SpinDegreesPerSecond = 25.0f;
	constexpr float FullRotationDegrees = 360.0f;
}

void SpinComponent::SetTargetComponentName(const char* aComponentName)
{
	myTargetComponentName = aComponentName;
}

void SpinComponent::BeginPlay()
{
	Transform* targetTransform = FindTargetTransform();
	if (targetTransform != nullptr)
	{
		myYaw = targetTransform->GetLocalRotationDegrees().x;
	}

}

void SpinComponent::Update(float deltaTime)
{
	myYaw += SpinDegreesPerSecond * deltaTime;
	myYaw = std::fmod(myYaw, FullRotationDegrees);

	if (Transform* transform = FindTargetTransform())
	{
		transform->SetLocalRotationDegrees(myYaw, 0, 0);
	}
}

Transform* SpinComponent::FindTargetTransform() const
{
	if (myTargetComponentName.empty())
	{
		return &GetOwner()->GetTransform();
	}

	Component* component = GetOwner()->FindComponent(myTargetComponentName);
	SceneComponent* targetComponent = dynamic_cast<SceneComponent*>(component);
	return targetComponent ? &targetComponent->GetTransform() : nullptr;
}
