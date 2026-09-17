#include "ModelViewerComponents.h"
#include "Application.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include <cmath>
#include "GameFramework/Scenes/References.h"
#include <utility>

namespace
{
	using Vector3f = CommonUtilities::Vector3f;
	void AimActorAlongCameraForward(Actor& anActor, const Transform& aCameraTransform)
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

// Owner-dependent initialization happens once, after scene validation.
void CameraControlsComponent::BeginPlay()
{
    const auto direction = GetOwner()->GetTransform().GetForward().GetNormalized();
    myYaw = std::atan2(direction.x, direction.z);
    myPitch = -std::asin(std::clamp(direction.y, -1.f, 1.f));
    GetOwner()->GetTransform().SetLocalRotationRadians(myYaw,myPitch,0);
}
void CameraControlsComponent::LateUpdate(float deltaTime)
{
    const auto& input = GetInput();
    auto& transform = GetOwner()->GetTransform();
    if (input.MouseLookActive)
    {
        myYaw += input.MouseDeltaX * .0025f;
        myPitch = std::clamp(myPitch + input.MouseDeltaY * .0025f, -1.55334303f, 1.55334303f);
        transform.SetLocalRotationRadians(myYaw,myPitch,0);
    }
    const auto forward = transform.GetForward().GetNormalized();
    const auto right = transform.GetRight().GetNormalized();
    Vector3f motion{};
    if (input.IsKeyDown(Keys::W)) motion += forward;
    if (input.IsKeyDown(Keys::S)) motion -= forward;
    if (input.IsKeyDown(Keys::D)) motion += right;
    if (input.IsKeyDown(Keys::A)) motion -= right;
    if (input.IsKeyDown(Keys::SPACE)) motion += Vector3f::UnitY;
    if (input.IsKeyDown(Keys::CONTROL)) motion -= Vector3f::UnitY;
    if (motion.LengthSqr() > 0) transform.SetLocalPosition(transform.GetLocalPosition() + motion.GetNormalized() * (500.f * deltaTime));
}
// Only this phase handles the R action. Handling it again in Update would observe
// the same physical press in both input domains and could toggle twice. Held motion
// uses dt; mouse deltas elsewhere are already accumulated movement, not a rate.
void SpinComponent::FixedUpdate(float deltaTime)
{
	if (GetInput().IsKeyPressed(Keys::R)) mySpinning = !mySpinning;
	if (!mySpinning) return;
	myYaw = std::fmod(myYaw + 25.0f * deltaTime, 360.0f);
	GetOwner()->SetRotation(myYaw, 0, 0);
}

void LightControlsComponent::ResolveReferences(References& context)
{
    auto camera = context.Require<CameraComponent>("Camera Actor", "Camera");
    if (auto* c = camera.Get()) myCamera = c->GetOwner()->GetHandle();
    myDirectional = context.Require<DirectionalLightComponent>("Directional Light Actor", "Directional Light");
    myPoints = { context.Require<PointLightComponent>("Warm Character Point Actor", "Warm Character Point Light") };
    mySpot = context.Require<SpotLightComponent>("Spot Light Actor", "Spot Light");
}
void AnimationControlsComponent::ResolveReferences(References& context)
{
    myMesh = context.Require<SkeletalMeshComponent>();
}

// Connect validates the required sibling once. Its handle can become empty if
// the mesh is removed during play. Number-pad 0-2 choose looping
// base clips; 3 requests a partial wave with a full-body fallback.
void AnimationControlsComponent::Update(float)

{
	const GameInput& anInputFrame = GetInput();
	auto* myAnimatedMeshComponent = myMesh.Get();
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
    auto* myCameraActor = myCamera.Get();
    auto* myDirectionalLightComponent = myDirectional.Get();
    auto* mySpotLightComponent = mySpot.Get();
    std::vector<PointLightComponent*> myPointLightComponents;
    for (const auto& point : myPoints) if (auto* live = point.Get()) myPointLightComponents.push_back(live);
	const GameInput& anInputFrame = GetInput();
	const bool shiftDown =
	    anInputFrame.IsKeyDown(Keys::SHIFT) || anInputFrame.IsKeyDown(Keys::LSHIFT) || anInputFrame.IsKeyDown(Keys::RSHIFT);

	if (anInputFrame.IsKeyPressed(Keys::P))
	{
		PrintLightTuningValues(myDirectionalLightComponent, myPointLightComponents, mySpotLightComponent);
	}

	if (shiftDown && myCameraActor != nullptr)
	{
		const Transform& cameraTransform = myCameraActor->GetTransform();
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
