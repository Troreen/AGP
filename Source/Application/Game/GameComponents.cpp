#include "GameComponents.h"
#include "GameLog.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "Maths.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	using Vector3f = CU::Vector3f;

	void AimActorAlongCameraForward(Actor& actor, const Transform& camera)
	{
		const auto forward = CU::NormalizeSafe(camera.GetLocalForward(), Vector3f::UnitZ);
		const float yaw = CU::RadiansToDegrees(std::atan2(forward.x, forward.z));
		const float pitch = CU::RadiansToDegrees(-std::asin(CU::Clamp(forward.y, -1.f, 1.f)));
		actor.GetTransform().SetLocalRotationDegrees(yaw, pitch, 0);
	}

	void PrintLightTuningValues(const DirectionalLightComponent* directional, const std::vector<PointLightComponent*>& points,
	                            const SpotLightComponent* spot)
	{
		if (directional)
		{
			const auto d = directional->GetWorldDirection();
			GAMELOG(Log, "Directional light direction: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}", d.x, d.y, d.z, directional->GetIntensity());
		}
		for (size_t i = 0; i < points.size(); ++i) if (points[i])
		{
			const auto p = points[i]->GetWorldPosition();
			GAMELOG(Log, "Point light {} position: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}, radius: {:.2f}", i, p.x, p.y, p.z,
			        points[i]->GetIntensity(), points[i]->GetRadius());
		}
		if (spot)
		{
			const auto p = spot->GetWorldPosition(); const auto d = spot->GetWorldDirection();
			GAMELOG(Log, "Spot light position: {{ {:.2f}, {:.2f}, {:.2f} }}, direction: {{ {:.2f}, {:.2f}, {:.2f} }}", p.x, p.y, p.z, d.x, d.y, d.z);
		}
	}
}

void CameraControlsComponent::BeginPlay()
{
	const auto direction = CU::NormalizeSafe(GetOwner()->GetTransform().GetLocalForward(), Vector3f::UnitZ);
	myYaw = std::atan2(direction.x, direction.z);
	myPitch = -std::asin(CU::Clamp(direction.y, -1.f, 1.f));
	auto& input = GetInputSystem();
	auto bindHeld = [this, &input](const InputActionId& action, bool& target)
	{
		mySubscriptions.push_back(input.Subscribe(action, [&target](const InputActionEvent& event) { target = event.Phase != InputActionPhase::Ended; }));
	};
	bindHeld(InputActions::CameraLookEnable, myLookActive);
	bindHeld(InputActions::CameraForward, myForward); bindHeld(InputActions::CameraBack, myBack);
	bindHeld(InputActions::CameraLeft, myLeft); bindHeld(InputActions::CameraRight, myRight);
	bindHeld(InputActions::CameraUp, myUp); bindHeld(InputActions::CameraDown, myDown);
	mySubscriptions.push_back(input.Subscribe(InputActions::CameraLookDelta, [this](const InputActionEvent& event)
	{
		if (event.Phase != InputActionPhase::Ended) myLookDelta += std::get<CommonUtilities::Vector2f>(event.Value);
	}));
}

void CameraControlsComponent::Update(float deltaTime)
{
	auto& transform = GetOwner()->GetTransform();
	if (myLookActive)
	{
		myYaw += myLookDelta.x * .0025f;
		myPitch = CU::Clamp(myPitch + myLookDelta.y * .0025f, CU::DegreesToRadians(-89.0f), CU::DegreesToRadians(89.0f));
		transform.SetLocalRotationDegrees(CU::RadiansToDegrees(myYaw), CU::RadiansToDegrees(myPitch), 0);
	}
	myLookDelta = {};
	Vector3f motion{};
	const auto forward = CU::NormalizeSafe(transform.GetLocalForward());
	const auto right = CU::NormalizeSafe(transform.GetLocalRight());
	if (myForward) motion += forward; if (myBack) motion -= forward;
	if (myRight) motion += right; if (myLeft) motion -= right;
	if (myUp) motion += Vector3f::UnitY; if (myDown) motion -= Vector3f::UnitY;
	if (motion.LengthSqr() > 0) transform.SetLocalPosition(transform.GetLocalPosition() + CU::NormalizeSafe(motion) * (500.f * deltaTime));
}

void SpinComponent::BeginPlay()
{
	myToggleSubscription = GetInputSystem().Subscribe(InputActions::ToggleSpin, [this](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started) mySpinning = !mySpinning;
	});
}

void SpinComponent::Update(float deltaTime)
{
	if (!mySpinning) return;
	myYaw = std::fmod(myYaw + 25.0f * deltaTime, 360.0f);
	GetOwner()->GetTransform().SetLocalRotationDegrees(myYaw, 0, 0);
}

void AnimationControlsComponent::BeginPlay()
{
	auto bind = [this](const InputActionId& action, const char* animation, bool partial)
	{
		mySubscriptions.push_back(GetInputSystem().Subscribe(action, [this, animation, partial](const InputActionEvent& event)
		{
			if (event.Phase != InputActionPhase::Started) return;
			if (auto* mesh = GetOwner()->GetComponent<SkeletalMeshComponent>())
				if (!partial || !mesh->PlayPartialAnimation(animation, false)) mesh->PlayAnimation(animation, !partial);
		}));
	};
	bind(InputActions::PlayBreathing, "Breathing", false); bind(InputActions::PlayWalk, "Walk", false);
	bind(InputActions::PlayRun, "Run", false); bind(InputActions::PlayWave, "Wave", true);
}

void LightControlsComponent::BeginPlay()
{
	auto bind = [this](const InputActionId& action, Request request)
	{
		mySubscriptions.push_back(GetInputSystem().Subscribe(action, [this, request](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Started) myRequests |= request;
		}));
	};
	bind(InputActions::PrintLights, Print); bind(InputActions::ToggleDirectional, ToggleDir);
	bind(InputActions::TogglePoint, TogglePoints); bind(InputActions::ToggleSpot, ToggleSpot);
	bind(InputActions::AimDirectional, AimDir); bind(InputActions::PlacePoint, PlacePoints); bind(InputActions::PlaceSpot, PlaceSpot);
}

void LightControlsComponent::Update(float)
{
	const unsigned requests = std::exchange(myRequests, 0u);
	if (!requests) return;
	auto* camera = GetWorld().FindActor(CameraName);
	auto* directionalActor = GetWorld().FindActor(DirectionalName);
	auto* directional = directionalActor ? directionalActor->GetComponent<DirectionalLightComponent>() : nullptr;
	auto* pointActor = GetWorld().FindActor(PointName);
	auto* point = pointActor ? pointActor->GetComponent<PointLightComponent>() : nullptr;
	auto* spotActor = GetWorld().FindActor(SpotName);
	auto* spot = spotActor ? spotActor->GetComponent<SpotLightComponent>() : nullptr;
	std::vector<PointLightComponent*> points; if (point) points.push_back(point);
	if (requests & Print) PrintLightTuningValues(directional, points, spot);
	if ((requests & ToggleDir) && directional) directional->SetEnabled(!directional->IsEnabled());
	if ((requests & TogglePoints) && point) point->SetEnabled(!point->IsEnabled());
	if ((requests & ToggleSpot) && spot) spot->SetEnabled(!spot->IsEnabled());
	if (!camera) return;
	const auto& cameraTransform = camera->GetTransform();
	if ((requests & AimDir) && directional) AimActorAlongCameraForward(*directional->GetOwner(), cameraTransform);
	if ((requests & PlacePoints) && point) point->GetOwner()->GetTransform().SetLocalPosition(cameraTransform.GetWorldPosition());
	if ((requests & PlaceSpot) && spot)
	{
		spot->GetOwner()->GetTransform().SetLocalPosition(cameraTransform.GetWorldPosition());
		AimActorAlongCameraForward(*spot->GetOwner(), cameraTransform);
	}
}
