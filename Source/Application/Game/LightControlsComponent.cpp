#include "LightControlsComponent.h"

#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/World/World.h"
#include "GameLog.h"
#include "Maths.hpp"

#include <cassert>
#include <cmath>
#include <utility>

namespace
{
	using Vector3f = CU::Vector3f;

	void AimActorAlongCameraForward(Actor& actor, const Transform& camera)
	{
		const Vector3f forward = CU::NormalizeSafe(camera.GetLocalForward(), Vector3f::UnitZ);
		const float yaw = CU::RadiansToDegrees(std::atan2(forward.x, forward.z));
		const float pitch = CU::RadiansToDegrees(-std::asin(CU::Clamp(forward.y, -1.0f, 1.0f)));
		actor.GetTransform().SetLocalRotationDegrees(yaw, pitch, 0);
	}

	void PrintLightTuningValues(const DirectionalLightComponent* directional,
		const std::vector<PointLightComponent*>& points, const SpotLightComponent* spot)
	{
		if (directional)
		{
			const Vector3f direction = directional->GetWorldDirection();
			GAMELOG(Log, "Directional light direction: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}",
				direction.x, direction.y, direction.z, directional->GetIntensity());
		}

		for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex)
		{
			const PointLightComponent* point = points[pointIndex];
			if (!point)
			{
				continue;
			}

			const Vector3f position = point->GetWorldPosition();
			GAMELOG(Log, "Point light {} position: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}, radius: {:.2f}",
				pointIndex, position.x, position.y, position.z, point->GetIntensity(), point->GetRadius());
		}

		if (spot)
		{
			const Vector3f position = spot->GetWorldPosition();
			const Vector3f direction = spot->GetWorldDirection();
			GAMELOG(Log, "Spot light position: {{ {:.2f}, {:.2f}, {:.2f} }}, direction: {{ {:.2f}, {:.2f}, {:.2f} }}",
				position.x, position.y, position.z, direction.x, direction.y, direction.z);
		}
	}
}

void LightControlsComponent::BeginPlay()
{
	auto bind = [this](const InputActionId& action, Request request)
	{
		mySubscriptions.push_back(GetInputSystem().Subscribe(action, [this, request](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Started)
			{
				myRequests |= request;
			}
		}));
	};

	bind(InputActions::PrintLights, Print);
	bind(InputActions::ToggleDirectional, ToggleDir);
	bind(InputActions::TogglePoint, TogglePoints);
	bind(InputActions::ToggleSpot, ToggleSpot);
	bind(InputActions::AimDirectional, AimDir);
	bind(InputActions::PlacePoint, PlacePoints);
	bind(InputActions::PlaceSpot, PlaceSpot);
}

void LightControlsComponent::Update(float)
{
	const unsigned requests = std::exchange(myRequests, 0u);
	if (!requests)
	{
		return;
	}

	Actor* directionalLightActor = GetWorld().FindActor(DirectionalName);
	DirectionalLightComponent* directionalLightComponent = directionalLightActor
		? directionalLightActor->GetComponent<DirectionalLightComponent>()
		: nullptr;

	Actor* spotLightActor = GetWorld().FindActor(SpotName);
	SpotLightComponent* spotLightComponent = spotLightActor ? spotLightActor->GetComponent<SpotLightComponent>() : nullptr;

	Actor* pointLightActor = GetWorld().FindActor(PointName);
	PointLightComponent* pointLightComponent = pointLightActor ? pointLightActor->GetComponent<PointLightComponent>() : nullptr;
	std::vector<PointLightComponent*> pointLightComponents;
	if (pointLightComponent)
	{
		pointLightComponents.push_back(pointLightComponent);
	}

	if (requests & Print)
	{
		PrintLightTuningValues(directionalLightComponent, pointLightComponents, spotLightComponent);
	}
	if ((requests & ToggleDir) && directionalLightComponent)
	{
		directionalLightComponent->SetEnabled(!directionalLightComponent->IsEnabled());
	}
	if ((requests & TogglePoints) && pointLightComponent)
	{
		pointLightComponent->SetEnabled(!pointLightComponent->IsEnabled());
	}
	if ((requests & ToggleSpot) && spotLightComponent)
	{
		spotLightComponent->SetEnabled(!spotLightComponent->IsEnabled());
	}

	Actor* cameraActor = GetWorld().FindActor(CameraName);
	assert(cameraActor != nullptr && "LightControlsComponent could not find the camera actor");
	const Transform& cameraTransform = cameraActor->GetTransform();

	if ((requests & AimDir) && directionalLightComponent)
	{
		AimActorAlongCameraForward(*directionalLightComponent->GetOwner(), cameraTransform);
	}
	if ((requests & PlacePoints) && pointLightComponent)
	{
		pointLightComponent->GetOwner()->GetTransform().SetLocalPosition(cameraTransform.GetWorldPosition());
	}
	if ((requests & PlaceSpot) && spotLightComponent)
	{
		spotLightComponent->GetOwner()->GetTransform().SetLocalPosition(cameraTransform.GetWorldPosition());
		AimActorAlongCameraForward(*spotLightComponent->GetOwner(), cameraTransform);
	}
}
