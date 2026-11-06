#include "PlayerController.h"
#include "PlayerMovementComponent.h"
#include <GameFramework/Components/AnimatorComponent.h>
#include "AbilityComponent.h"
#include "GameFramework/AudioManager.h"
#include <GameFramework/ServiceLocator.h>
#include <CommonUtilities/InputMapper.h>
#include <CommonUtilities/InputHandler.h>
#include <CommonUtilities/EnumKeyCode.h>
#include <GameFramework/Components/CameraComponent.h>
#include <Intersection.hpp>
#include <GraphicsEngine/GraphicsEngine.h>
#include <GameFramework/World/Actor.h>
#include <iostream>
#include <GameFramework/Scenes/CustomComponentVariables.h>
#include <Player/Abilities/Ability.h>
#include <AISystems/Pathfinder.hpp>


PlayerController::PlayerController()
{}

PlayerController::~PlayerController()
{
	CommonUtilities::InputMapper& inputMapper = ServiceLocator::GetInstance().GetInputMapper();
	if (myNavigationEventId > 0)
	{
		inputMapper.RemoveEventListener(myNavigationEventId);
	}

}

void PlayerController::InitializeCustomVariables(const CustomComponentVariables& someVariables)
{
	someVariables;
}

void PlayerController::BeginPlay()
{
	CommonUtilities::InputMapper& inputMapper = ServiceLocator::GetInstance().GetInputMapper();
	inputMapper.BindActionToInputCode("PLAYER_NAVIGATION", EKeyCode::MOUSELBUTTON);
	myNavigationEventId = inputMapper.AddEventListener("PLAYER_NAVIGATION", [this](const CommonUtilities::InputEvent& aEvent) { NavigationHandler(aEvent); });

	ServiceLocator::GetInstance().SetPlayer(GetOwner());

	myMovement = GetOwner()->GetComponent<PlayerMovementComponent>();

	if (!myMovement)
	{
		std::cout << "NO MOVEMENT WAS FOUND! - PlayerController \n";
		return;
	}

	myAbility = GetOwner()->GetComponent<AbilityComponent>();

	if (!myAbility)
	{
		std::cout << "NO ABILITY WAS FOUND! - PlayerController \n";
		return;
	}

	myCamera = ServiceLocator::GetInstance().GetActiveCamera();

	if (!myCamera)
	{
		std::cout << "NO CAMERA WAS FOUND! - PlayerController \n";
		return;
	}

	Actor* cameraActor = myCamera->GetOwner();

	if (cameraActor)
	{
		if (myCamera->GetOwner()->GetName() == "BP_Camera")
		{
			const Transform& cameraTransform = myCamera->GetTransform();
			cameraActor->GetTransform().SetLocalPosition(cameraTransform.GetLocalPosition());
			cameraActor->GetTransform().SetLocalRotation(cameraTransform.GetLocalRotation());
		}
		myCameraOffset = cameraActor->GetTransform().GetWorldPosition() - GetOwner()->GetTransform().GetWorldPosition();
	}

	AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>();
	myAnimationEventIds.emplace_back("Running", 2, animation->GetTree().GetAnimationState("Running").OnFrameEvent().Subscribe([](unsigned, const std::string&)
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eFootstepOne);
		}));

	myAnimationEventIds.emplace_back("Dodge", 0, animation->GetTree().GetAnimationState("Dodge").OnEnterEvent().Subscribe([this]()
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eDodge);
		}));

	myAnimationEventIds.emplace_back("KB1", 0, animation->GetTree().GetAnimationState("KB1").OnEnterEvent().Subscribe([this]()
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eAbilityOne);

			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(true);
			}
		}));

	myAnimationEventIds.emplace_back("KB1", 1, animation->GetTree().GetAnimationState("KB1").OnExitEvent().Subscribe([this]()
			{
				if (myMovement)
				{
					myMovement->SetAbilityMovementLocked(false);
				}
			}));

	myAnimationEventIds.emplace_back("KB2", 0, animation->GetTree().GetAnimationState("KB2").OnEnterEvent().Subscribe([this]()
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eAbilityTwo);
		}));

	myAnimationEventIds.emplace_back("KB3", 0, animation->GetTree().GetAnimationState("KB3").OnEnterEvent().Subscribe([this]()
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eAbilityThree);

			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(true);
			}
		}));

	myAnimationEventIds.emplace_back("KB3", 1, animation->GetTree().GetAnimationState("KB3").OnExitEvent().Subscribe([this]()
			{
				if (myMovement)
				{
					myMovement->SetAbilityMovementLocked(false);
				}
			}));


	myAnimationEventIds.emplace_back("KB4", 0, animation->GetTree().GetAnimationState("KB4").OnEnterEvent().Subscribe([this]()
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eAbilityFour);

			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(true);
			}
		}));

	myAnimationEventIds.emplace_back("KB4", 1, animation->GetTree().GetAnimationState("KB4").OnExitEvent().Subscribe([this]()
		{
			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(false);
			}
		}));

	myAnimationEventIds.emplace_back("FirstMB1", 0, animation->GetTree().GetAnimationState("FirstMB1").OnEnterEvent().Subscribe([this]()
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eMouseButtonOne);

			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(true);
			}
		}));

	myAnimationEventIds.emplace_back("FirstMB1", 1, animation->GetTree().GetAnimationState("FirstMB1").OnExitEvent().Subscribe([this]()
		{
			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(false);
			}
		}));

	myAnimationEventIds.emplace_back("SecondMB1", 0, animation->GetTree().GetAnimationState("SecondMB1").OnEnterEvent().Subscribe([this]()
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eMouseButtonOne);

			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(true);
			}
		}));

	myAnimationEventIds.emplace_back("SecondMB1", 1, animation->GetTree().GetAnimationState("SecondMB1").OnExitEvent().Subscribe([this]()
		{
			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(false);
			}
		}));

	myAnimationEventIds.emplace_back("MB2", 0, animation->GetTree().GetAnimationState("MB2").OnEnterEvent().Subscribe([this]()
		{
			ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eMouseButtonTwo);

			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(true);
			}
		}));

	myAnimationEventIds.emplace_back("MB2", 1, animation->GetTree().GetAnimationState("MB2").OnExitEvent().Subscribe([this]()
		{
			if (myMovement)
			{
				myMovement->SetAbilityMovementLocked(false);
			}
		}));
}

void PlayerController::EndPlay() noexcept
{
	AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>();
	if (animation != nullptr)
	{
		for (size_t i = 0; i < myAnimationEventIds.size(); i++)
		{
			switch (std::get<unsigned>(myAnimationEventIds[i]))
			{
			case 0:
				animation->GetTree().GetAnimationState(
					std::get<std::string>(myAnimationEventIds[i])).OnEnterEvent().Unsubscribe(std::get<ListenerIdentification>(myAnimationEventIds[i]));
				break;

			case 1:
				animation->GetTree().GetAnimationState(
					std::get<std::string>(myAnimationEventIds[i])).OnExitEvent().Unsubscribe(std::get<ListenerIdentification>(myAnimationEventIds[i]));
				break;

			case 2:
				animation->GetTree().GetAnimationState(
					std::get<std::string>(myAnimationEventIds[i])).OnFrameEvent().Unsubscribe(std::get<ListenerIdentification>(myAnimationEventIds[i]));
				break;
			}
		}
	}
}

void PlayerController::Update(float aDeltaTime)
{
	aDeltaTime;
	if (!myCamera)
	{
		myCamera = ServiceLocator::GetInstance().GetActiveCamera();
		Actor* cameraActor = myCamera->GetOwner();

		if (cameraActor)
		{
			myCameraOffset = cameraActor->GetTransform().GetWorldPosition() - GetOwner()->GetTransform().GetWorldPosition();
		}
	}

	CommonUtilities::Vector2i inputMousePos = ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->GetMousePosition();

	CommonUtilities::Vector2f mousePos =
	{
		static_cast<float>(inputMousePos.x),
		static_cast<float>(inputMousePos.y)
	};

	const Transform& transform = GetOwner()->GetTransform();

	if (!myScriptedControlLocked && ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyPressed(static_cast<int>(EKeyCode::TWO)))
	{
		const CommonUtilities::Vector2u& clientSize = GraphicsEngine::Get().GetClientSize();

		CommonUtilities::Ray<float> ray = myCamera->ScreenPointToRay(mousePos, clientSize);

		CommonUtilities::Vector3f outIntersection;

		CommonUtilities::Plane<float> plane;

		plane.InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });

		if (CommonUtilities::IntersectionPlaneRay(plane, ray, outIntersection))
		{
			if (myAbility)
			{
				AbilityActivationContext context = {};

				CommonUtilities::Vector3f position = transform.GetLocalPosition();

				outIntersection.y = position.y;

				CommonUtilities::Vector3f direction = outIntersection - position;
				context.Direction = direction.GetNormalized();
				myAbility->TryActivateAbility(AbilitySlot::Slot2, context);
			}
		}
	}

	if (!myScriptedControlLocked && ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyPressed(static_cast<int>(EKeyCode::SPACE)))
	{
		const CommonUtilities::Vector2u& clientSize = GraphicsEngine::Get().GetClientSize();

		CommonUtilities::Ray<float> ray = myCamera->ScreenPointToRay(mousePos, clientSize);

		CommonUtilities::Vector3f outIntersection;

		CommonUtilities::Plane<float> plane;

		plane.InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });

		if (CommonUtilities::IntersectionPlaneRay(plane, ray, outIntersection))
		{
			if (myAbility)
			{
				AbilityActivationContext context = {};

				CommonUtilities::Vector3f position = transform.GetLocalPosition();

				outIntersection.y = position.y;

				CommonUtilities::Vector3f direction = outIntersection - position;
				context.Direction = direction.GetNormalized();
				myAbility->TryActivateAbility(AbilitySlot::Space, context);
			}
		}
	}


	//if (!myScriptedControlLocked && ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyDown(static_cast<int>(EKeyCode::MOUSELBUTTON)))
	//{
	//	//TODO: Movement
	//	//Send mouse pos to camera
	//	//Create ray with picking
	//	//Intersection with plane - Should be handled by Collisions later
	//	//Get intersected position
	//	//Send to movement
	//	//Movement then moves to that position.


	//	if (myMovement)
	//	{
	//		if (myMovement->CanTakeMovementInput())
	//		{
	//			const CommonUtilities::Vector2u& clientSize = GraphicsEngine::Get().GetClientSize();

	//			CommonUtilities::Ray<float> ray = myCamera->ScreenPointToRay(mousePos, clientSize);

	//			std::vector<CommonUtilities::Vector3f> waypoints;

	//			ServiceLocator::GetInstance().GetPathFinder().ImmediateRequest(transform.GetLocalPosition(), ray, waypoints);

	//			myMovement->SetWaypoints(waypoints);
	//		}
	//	}


	//	/*CommonUtilities::Vector3f outIntersection;

	//	CommonUtilities::Plane<float> plane;

	//	plane.InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });

	//	if (CommonUtilities::IntersectionPlaneRay(plane, ray, outIntersection))
	//	{
	//		if (myMovement)
	//		{
	//			if (myMovement->CanTakeMovementInput())
	//			{
	//				myMovement->SetDestination(outIntersection);
	//			}
	//		}
	//	}*/


	//	//TODO: Later add pathfinding with navmesh
	//}

	if (!myScriptedControlLocked && ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyPressed(static_cast<int>(EKeyCode::ONE)))
	{
		if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
		{
			animation->GetTree().SetTrigger("IsKB1");
		}
	}

	if (!myScriptedControlLocked && ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyPressed(static_cast<int>(EKeyCode::THREE)))
	{
		if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
		{
			animation->GetTree().SetTrigger("IsKB3");
		}
	}

	if (!myScriptedControlLocked && ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyPressed(static_cast<int>(EKeyCode::FOUR)))
	{
		if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
		{
			animation->GetTree().SetTrigger("IsKB4");
		}
	}

	if (!myScriptedControlLocked && ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyDown(static_cast<int>(EKeyCode::MOUSELBUTTON)) && ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyDown(static_cast<int>(EKeyCode::SHIFT)))
	{
		if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
		{
			animation->GetTree().SetTrigger("IsFirstMB1");
		}
	}

	if (!myScriptedControlLocked && (ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyPressed(static_cast<int>(EKeyCode::MOUSERBUTTON))))
	{
		if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
		{
			animation->GetTree().SetTrigger("IsMB2");
		}
	}

	myCamera->GetOwner()->GetTransform().SetLocalPosition(transform.GetWorldPosition() + myCameraOffset);
}

void PlayerController::SetScriptedControlLocked(bool aLocked)
{
	myScriptedControlLocked = aLocked;
	if (aLocked)
	{
		if (!myMovement)
		{
			myMovement = GetOwner()->GetComponent<PlayerMovementComponent>();
		}
	}
}

bool PlayerController::IsScriptedControlLocked() const
{
	return myScriptedControlLocked;
}

void PlayerController::NavigationHandler(const CommonUtilities::InputEvent& aEvent)
{
	if (aEvent.inputData.isHeld && myMovement && myMovement->CanTakeMovementInput())
	{
		const CommonUtilities::Vector2f mousePos{ aEvent.inputData.valueA, aEvent.inputData.valueB };
		const CommonUtilities::Vector2u& clientSize = GraphicsEngine::Get().GetClientSize();

		CommonUtilities::Ray<float> ray = myCamera->ScreenPointToRay(mousePos, clientSize);

		std::vector<CommonUtilities::Vector3f> waypoints;

		ServiceLocator::GetInstance().GetPathFinder().ImmediateRequest(GetOwner()->GetTransform().GetLocalPosition(), ray, waypoints);

		myMovement->SetWaypoints(std::move(waypoints));
	}
}
