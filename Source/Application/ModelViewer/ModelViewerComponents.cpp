#include "ModelViewerComponents.h"
#include "Application.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include <cmath>
#include <utility>

namespace
{
	using Vector3f = CommonUtilities::Vector3f;
	void AimActorAlongCameraForward(Actor& anActor, const CommonUtilities::Transform& aCameraTransform)
	{
		Vector3f forward = aCameraTransform.GetForward();
		if (forward.LengthSqr() <= 0.0f)
		{
			forward = Vector3f::UnitZ;
		}
		else
		{
			forward.Normalize();
		}

		const Vector3f position = anActor.GetTransform().GetPosition();
		anActor.LookAt(position + forward);
	}

	void PrintLightTuningValues(const DirectionalLightComponent* aDirectionalLightComponent,
	                            const std::vector<PointLightComponent*>& somePointLightComponents,
	                            const SpotLightComponent* aSpotLightComponent)
	{
		unsigned activeLightCount = 0;
		if (aDirectionalLightComponent != nullptr)
		{
			activeLightCount += aDirectionalLightComponent->IsEnabled() ? 1 : 0;
			const Vector3f direction = aDirectionalLightComponent->GetWorldDirection();
			MVLOG(Log, "Directional light direction: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}", direction.x, direction.y,
			      direction.z, aDirectionalLightComponent->GetIntensity());
		}

		for (size_t pointIndex = 0; pointIndex < somePointLightComponents.size(); ++pointIndex)
		{
			const PointLightComponent* pointLightComponent = somePointLightComponents[pointIndex];
			if (pointLightComponent == nullptr)
			{
				continue;
			}

			activeLightCount += pointLightComponent->IsEnabled() ? 1 : 0;
			const Vector3f position = pointLightComponent->GetWorldPosition();
			MVLOG(Log, "Point light {} position: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}, radius: {:.2f}", pointIndex, position.x,
			      position.y, position.z, pointLightComponent->GetIntensity(), pointLightComponent->GetRadius());
		}

		if (aSpotLightComponent != nullptr)
		{
			activeLightCount += aSpotLightComponent->IsEnabled() ? 1 : 0;
			const Vector3f position = aSpotLightComponent->GetWorldPosition();
			const Vector3f direction = aSpotLightComponent->GetWorldDirection();
			MVLOG(
			    Log,
			    "Spot light position: {{ {:.2f}, {:.2f}, {:.2f} }}, direction: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}, radius: {:.2f}",
			    position.x, position.y, position.z, direction.x, direction.y, direction.z, aSpotLightComponent->GetIntensity(),
			    aSpotLightComponent->GetRadius());
		}

		MVLOG(Log, "Active demo lights: {}", activeLightCount);
	}

}

// Lazy owner-dependent setup: GetOwner() was unavailable in the constructor.
// After initializing the controller, consume the facade's copied input. No Win32
// polling, locks, frame scheduling or rendering calls belong in this component.
void CameraControlsComponent::LateUpdate(float deltaTime)
{
	if (!myInitialized)
	{
		myController.Init(GetOwner()->GetTransform());
		myInitialized = true;
	}
	const GameInput& input = myContext.GetInput();
	FreeFlyCameraController::InputState camera;
	camera.MoveForward = input.IsKeyDown(Keys::W);
	camera.MoveBackward = input.IsKeyDown(Keys::S);
	camera.MoveRight = input.IsKeyDown(Keys::D);
	camera.MoveLeft = input.IsKeyDown(Keys::A);
	camera.MoveUp = input.IsKeyDown(Keys::SPACE);
	camera.MoveDown = input.IsKeyDown(Keys::CONTROL);
	camera.MouseLookActive = input.MouseLookActive;
	camera.MouseDeltaX = input.MouseDeltaX;
	camera.MouseDeltaY = input.MouseDeltaY;
	myController.Update(deltaTime, camera);
}

// Only this phase handles the R action. Handling it again in Update would observe
// the same physical press in both input domains and could toggle twice. Held motion
// uses dt; mouse deltas elsewhere are already accumulated movement, not a rate.
void SpinComponent::FixedUpdate(float deltaTime)
{
	if (myContext.GetInput().IsKeyPressed(Keys::R)) mySpinning = !mySpinning;
	if (!mySpinning) return;
	myYaw = std::fmod(myYaw + 25.0f * deltaTime, 360.0f);
	GetOwner()->SetRotation(myYaw, 0, 0);
}

LightControlsComponent::LightControlsComponent(GameContext& context, Actor* camera,
	DirectionalLightComponent* directional, std::vector<PointLightComponent*> points, SpotLightComponent* spot)
	: myContext(context), myCameraActor(camera), myDirectionalLightComponent(directional),
	  myPointLightComponents(std::move(points)), mySpotLightComponent(spot)
{
}

// Find the sibling after attachment, when the scene is fully constructed. Missing
// render components simply make the control inert. Number-pad 0-2 choose looping
// base clips; 3 requests a partial wave with a full-body fallback.
void AnimationControlsComponent::Update(float)

{
	const GameInput& anInputFrame = myContext.GetInput();
	auto* myAnimatedMeshComponent = GetOwner()->GetComponent<SkeletalMeshComponent>();
	if (myAnimatedMeshComponent == nullptr)
	{
		return;
	}

	if (anInputFrame.IsKeyPressed(Keys::NUMPAD0))
	{
		myAnimatedMeshComponent->PlayAnimation("Breathing", true);
	}

	if (anInputFrame.IsKeyPressed(Keys::NUMPAD1))
	{
		myAnimatedMeshComponent->PlayAnimation("Walk", true);
	}

	if (anInputFrame.IsKeyPressed(Keys::NUMPAD2))
	{
		myAnimatedMeshComponent->PlayAnimation("Run", true);
	}

	if (anInputFrame.IsKeyPressed(Keys::NUMPAD3))
	{
		if (!myAnimatedMeshComponent->PlayPartialAnimation("Wave", false))
		{
			myAnimatedMeshComponent->PlayAnimation("Wave", false);
		}
	}
}

// Use the final camera pose for Shift+7/8/9 aiming/placement. Without Shift, the
// same keys toggle the corresponding light group. Only mutate gameplay properties;
// the host publishes those values after all late updates finish.
void LightControlsComponent::LateUpdate(float)
{
	const GameInput& anInputFrame = myContext.GetInput();
	const bool shiftDown =
	    anInputFrame.IsKeyDown(Keys::SHIFT) || anInputFrame.IsKeyDown(Keys::LSHIFT) || anInputFrame.IsKeyDown(Keys::RSHIFT);

	if (anInputFrame.IsKeyPressed(Keys::P))
	{
		PrintLightTuningValues(myDirectionalLightComponent, myPointLightComponents, mySpotLightComponent);
	}

	if (shiftDown && myCameraActor != nullptr)
	{
		const CommonUtilities::Transform& cameraTransform = myCameraActor->GetTransform();
		const Vector3f cameraPosition = cameraTransform.GetPosition();

		if ((anInputFrame.IsKeyPressed(Keys::NUMPAD7) || anInputFrame.KeysPressed[static_cast<size_t>('7')]) &&
		    myDirectionalLightComponent != nullptr)
		{
			if (Actor* lightActor = myDirectionalLightComponent->GetOwner())
			{
				AimActorAlongCameraForward(*lightActor, cameraTransform);
				const Vector3f direction = myDirectionalLightComponent->GetWorldDirection();
				MVLOG(Log, "Aimed directional light from camera direction: {{ {:.2f}, {:.2f}, {:.2f} }}", direction.x, direction.y,
				      direction.z);
			}
			return;
		}

		if (anInputFrame.IsKeyPressed(Keys::NUMPAD8) || anInputFrame.KeysPressed[static_cast<size_t>('8')])
		{
			for (PointLightComponent* pointLightComponent : myPointLightComponents)
			{
				if (pointLightComponent == nullptr)
				{
					continue;
				}

				if (Actor* lightActor = pointLightComponent->GetOwner())
				{
					lightActor->SetPosition(cameraPosition);
					MVLOG(Log, "Moved point light to camera position: {{ {:.2f}, {:.2f}, {:.2f} }}", cameraPosition.x, cameraPosition.y,
					      cameraPosition.z);
					break;
				}
			}
			return;
		}

		if ((anInputFrame.IsKeyPressed(Keys::NUMPAD9) || anInputFrame.KeysPressed[static_cast<size_t>('9')]) &&
		    mySpotLightComponent != nullptr)
		{
			if (Actor* lightActor = mySpotLightComponent->GetOwner())
			{
				lightActor->SetPosition(cameraPosition);
				AimActorAlongCameraForward(*lightActor, cameraTransform);
				const Vector3f direction = mySpotLightComponent->GetWorldDirection();
				MVLOG(
				    Log,
				    "Moved spot light to camera and aimed forward. Position: {{ {:.2f}, {:.2f}, {:.2f} }}, direction: {{ {:.2f}, {:.2f}, {:.2f} }}",
				    cameraPosition.x, cameraPosition.y, cameraPosition.z, direction.x, direction.y, direction.z);
			}
			return;
		}
	}

	if ((anInputFrame.IsKeyPressed(Keys::NUMPAD7) || anInputFrame.KeysPressed[static_cast<size_t>('7')]) &&
	    myDirectionalLightComponent != nullptr)
	{
		myDirectionalLightComponent->SetEnabled(!myDirectionalLightComponent->IsEnabled());
	}

	if (anInputFrame.IsKeyPressed(Keys::NUMPAD8) || anInputFrame.KeysPressed[static_cast<size_t>('8')])
	{
		bool shouldEnable = true;
		bool foundPointLight = false;
		for (const PointLightComponent* pointLightComponent : myPointLightComponents)
		{
			if (pointLightComponent != nullptr)
			{
				shouldEnable = !pointLightComponent->IsEnabled();
				foundPointLight = true;
				break;
			}
		}

		if (foundPointLight)
		{
			for (PointLightComponent* pointLightComponent : myPointLightComponents)
			{
				if (pointLightComponent != nullptr)
				{
					pointLightComponent->SetEnabled(shouldEnable);
				}
			}
		}
	}

	if ((anInputFrame.IsKeyPressed(Keys::NUMPAD9) || anInputFrame.KeysPressed[static_cast<size_t>('9')]) &&
	    mySpotLightComponent != nullptr)
	{
		mySpotLightComponent->SetEnabled(!mySpotLightComponent->IsEnabled());
	}
}

