#include "PlayerController.h"
#include "PlayerMovementComponent.h"
#include <GameFramework/ServiceLocator.h>
#include <CommonUtilities/InputMapper.h>
#include <CommonUtilities/InputHandler.h>
#include <CommonUtilities/EnumKeyCode.h>
#include <GameFramework/Components/CameraComponent.h>
#include <Intersection.hpp>
#include <GraphicsEngine/GraphicsEngine.h>
#include <GameFramework/World/Actor.h>
#include <iostream>

PlayerController::PlayerController()
{}

void PlayerController::BeginPlay()
{
	ServiceLocator::GetInstance().SetPlayer(GetOwner());

	myMovement = GetOwner()->GetComponent<PlayerMovementComponent>();

	if (!myMovement)
	{
		std::cout << "NO MOVEMENT WAS FOUND! - Player \n";
		return;
	}

	myCamera = &ServiceLocator::GetInstance().GetActiveCamera();

	if (!myCamera)
	{
		std::cout << "NO CAMERA WAS FOUND! - Player \n";
		return;
	}

	Actor* cameraActor = myCamera->GetOwner();

	if (cameraActor)
	{
		myCameraOffset = cameraActor->GetTransform().GetWorldPosition() - GetOwner()->GetTransform().GetWorldPosition();
	}

}

void PlayerController::Update(float aDeltaTime)
{
	if (!myCamera)
	{
		myCamera = &ServiceLocator::GetInstance().GetActiveCamera();
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

	if (ServiceLocator::GetInstance().GetInputMapper().GetInputHandler()->IsKeyPressed(static_cast<int>(EKeyCode::MOUSELBUTTON)))
	{
		//TODO: Movement
		//Send mouse pos to camera
		//Create ray with picking
		//Intersection with plane - Should be handled by Collisions later
		//Get intersected position
		//Send to movement
		//Movement then moves to that position.

		const CommonUtilities::Vector2u& clientSize = GraphicsEngine::Get().GetClientSize();

		CommonUtilities::Vector2f normalizedMousePos =
		{
			mousePos.x / static_cast<float>(clientSize.x),
			mousePos.y / static_cast<float>(clientSize.y)
		};
		CommonUtilities::Ray<float> ray = myCamera->ScreenPointToRay(normalizedMousePos);

		CommonUtilities::Vector3f outIntersection;

		CommonUtilities::Plane<float> plane;

		plane.InitWithPointAndNormal({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });

		if (CommonUtilities::IntersectionPlaneRay(plane, ray, outIntersection))
		{
			if (myMovement)
			{
				myMovement->SetDestination(outIntersection);
			}
		}


		//TODO: Later add pathfinding with navmesh
	}

	Transform& transform = GetOwner()->GetTransform();

	myCamera->GetOwner()->GetTransform().SetLocalPosition(transform.GetWorldPosition() + myCameraOffset);
}
