#include "SpinComponent.h"

#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/Scenes/CustomComponentProperties.h"

#include <cmath>
#include <stdexcept>

namespace
{
	constexpr float FullRotationDegrees = 360.0f;
}

void SpinComponent::SetTargetComponentName(const char* aComponentName)
{
	myTargetComponentName = aComponentName;
}

void SpinComponent::SetDegreesPerSecond(float aDegreesPerSecond)
{
	if (!std::isfinite(aDegreesPerSecond))
	{
		throw std::invalid_argument("Rotation speed must be finite");
	}
	myDegreesPerSecond = aDegreesPerSecond;
}

void SpinComponent::Configure(const CustomComponentProperties& someProperties)
{
	const float spinSpeed = someProperties.Get<float>("SpinSpeed", myDegreesPerSecond);
	const std::string targetComponentName = someProperties.Get<std::string>("TargetComponentName", myTargetComponentName);
	SetDegreesPerSecond(spinSpeed);
	SetTargetComponentName(targetComponentName.c_str());
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
	myYaw += myDegreesPerSecond * deltaTime;
	myYaw = std::fmod(myYaw, FullRotationDegrees);

	if (Transform* transform = FindTargetTransform())
	{
		const auto rotation = transform->GetLocalRotationDegrees();
		transform->SetLocalRotationDegrees(myYaw, rotation.y, rotation.z);
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
