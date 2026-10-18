#include "Game.h"
#include "Runtime/GameApplication.h"
#include "GameLog.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/AnimationAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/AssetHandling/TextureAsset.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/AnimatorComponent.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/SpriteComponent.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/World/World.h"
#include <GameFramework/Animation/AnimationManager.h>
#include "InputMapper.h"
#include "Player/PlayerActor.h"
#include "Player/PlayerController.h"
#include "Player/PlayerMovementComponent.h"
#include "BehaviourTree/BehaviourTreeComponent.h"
#include "Components/CollisionDemoComponent.h"
#include "GameFramework/Components/ColliderComponent.h"

#include <memory>

DEFINE_LOG_CATEGORY(LogGame);

namespace
{
	// Temporary debug setup for trying collision overlaps. Leave it off during normal play.
	constexpr bool EnableCollisionDemo = false;

	// Give each debug sphere its own Actor so the demo can position it independently.
	ColliderComponent* SpawnDebugCollisionSphere(World& world, const char* actorName, float sphereRadius, unsigned collisionLayer)
	{
		Actor* sphereActor = world.SpawnActor(actorName);
		auto* sphereCollider = sphereActor->AddComponent<ColliderComponent>("Collider");
		sphereCollider->SetSphereRadius(sphereRadius);
		sphereCollider->SetLayer(collisionLayer);
		sphereCollider->SetMask(1u << 0);

		return sphereCollider;
	}
}

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize(GameApplication& anApplication)
{
	CommonUtilities::InputMapper& input = ServiceLocator::GetInstance().GetInputMapper();

	myInputListenerIDs.push_back(input.AddEventListener("ReloadScene", [&anApplication](const CommonUtilities::InputEvent& anEvent)
	{
		if (anEvent.inputData.isPressed)
		{
			anApplication.ReloadCurrentScene();
		}
	}));

	GAMELOG(Log, "Game ready: F4 reloads the current scene, ESC quits the game.");
}

void Game::Update(World& aWorld, [[maybe_unused]] float aDeltaTime)
{
	if (!ServiceLocator::GetInstance().GetAudioManager().IsEventPlaying(eMainTheme))
	
	{
		ServiceLocator::GetInstance().GetAudioManager().PlayMusic(eMainTheme);
	}

	// The game chooses to listen from the active view, including the debug camera.
	// Commenting this out becasue it spams the log with info about listener position
	// if (const CameraComponent* camera = aWorld.GetActiveCamera())
	// {
	// 	ServiceLocator::GetInstance().GetAudioManager().SetAudioListener(camera->GetWorldPosition());
	// }

}

void Game::Shutdown()
{
	CommonUtilities::InputMapper& input = ServiceLocator::GetInstance().GetInputMapper();
	for (unsigned id : myInputListenerIDs)
	{
		input.RemoveEventListener(id);
	}

	myInputListenerIDs.clear();
}

void Game::ConfigureWorld(World& aWorld)
{
	// {
	// 	Actor* playerBro = aWorld.SpawnActor("PLAYAH_BRO");
	// 	playerBro->GetTransform().SetLocalRotationDegrees({ 0, 0, 0 });
	// 	playerBro->GetTransform().SetWorldPosition({ 0, -15, 0 });

	//	SkeletalMeshComponent* component = playerBro->AddComponent<SkeletalMeshComponent>();
	//	AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();
	//	std::shared_ptr<MeshAsset> meshAsset = assetRegistry.GetAsset<MeshAsset>("SK_player.fbx");
	//	component->SetMesh(meshAsset);

	//	AnimatorComponent* animatorBro = playerBro->AddComponent<AnimatorComponent>();
	//	animatorBro->SetMeshComponent(component);
	//	animatorBro->SetTree("PlayerTree");


	//	animatorBro->GetTree().SetBool("IsIdle", true);
	//}

	//{
	//	Actor* playerBro = aWorld.SpawnActor("BRO");
	//	playerBro->GetTransform().SetLocalRotationDegrees({ 0, 0, 0 });
	//	playerBro->GetTransform().SetWorldPosition({ -100, 100, 0 });

	//	SkeletalMeshComponent* component = playerBro->AddComponent<SkeletalMeshComponent>();
	//	AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();
	//	std::shared_ptr<MeshAsset> meshAsset = assetRegistry.GetAsset<MeshAsset>("SK_C_Tga_Bro.fbx");
	//	component->SetMesh(meshAsset);

	//	AnimatorComponent* animatorBro = playerBro->AddComponent<AnimatorComponent>();
	//	animatorBro->SetMeshComponent(component);
	//	animatorBro->SetTree("BroTree");
	//	animatorBro->ConfigurePartialLayerFromJointName("RightArm");


	//	animatorBro->GetTree().SetBool("IsIdle", true);
	//}

	{
		Actor* actor = aWorld.SpawnActor("TGALOGO");
		SpriteComponent* component = actor->AddComponent<SpriteComponent>();

		AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();

		std::shared_ptr<TextureAsset> asset = assetRegistry.GetAsset<TextureAsset>("TgaLogo.dds");
		component->SetSprite(asset);
		component->SetSize({ 2048.0f * 0.5f, 512.0f * 0.5f });
		component->SetPivot({ 1, 1 });
		component->SetPosition({ 0.0f, 0.0f });
		component->SetAnchor({ 1.0f, 1.0f });
		//component->SendDrawCall();
	}

	{
		Actor* actor = aWorld.SpawnActor("Player");
		{
			actor->GetTransform().SetLocalPosition({0,0,0});
			SkeletalMeshComponent* component = actor->AddComponent<SkeletalMeshComponent>();
			AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();

			std::shared_ptr<MeshAsset> asset = assetRegistry.GetAsset<MeshAsset>("SK_player.fbx");
			component->SetMesh(asset);

			AnimatorComponent* animatorBro = actor->AddComponent<AnimatorComponent>();
			animatorBro->SetMeshComponent(component);
			animatorBro->SetTree("PlayerTree");


			animatorBro->GetTree().SetBool("IsIdle", true);
		}
		{
			actor->AddComponent<PlayerController>();
		}
		{
			actor->AddComponent<PlayerMovementComponent>();
		}
	}

	{
		Actor* actor = aWorld.SpawnActor("Enemy");
		{
			SkeletalMeshComponent* component = actor->AddComponent<SkeletalMeshComponent>();
			AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();

			std::shared_ptr<MeshAsset> asset = assetRegistry.GetAsset<MeshAsset>("SK_enemy02.fbx");
			component->SetMesh(asset);


			AnimatorComponent* animatorBro = actor->AddComponent<AnimatorComponent>();
			std::shared_ptr<AnimationAsset> animationIdle = assetRegistry.GetAsset<AnimationAsset>("A_enemy02_idle.fbx");
			std::shared_ptr<AnimationAsset> animationRun = assetRegistry.GetAsset<AnimationAsset>("A_enemy02_mainAttack.fbx");
			animatorBro->SetMeshComponent(component);
			animatorBro->AddAnimation("Idle", animationIdle);
			animatorBro->AddAnimation("Run", animationRun);
			animatorBro->PlayAnimation("Idle", true);

		}
		{
			actor->AddComponent<BehaviourTreeComponent>();
		}

		actor->GetTransform().SetData({ { -500, 0, 0 }, {180, 0, 0} });
	}

	//{
	//	Actor* actor = aWorld.SpawnActor("NavmeshTest");
	//	SkeletalMeshComponent* component = actor->AddComponent<SkeletalMeshComponent>();
	//	actor->GetTransform().SetWorldPosition({ -100, 100, 0 });

	//	AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();

	//	std::shared_ptr<MeshAsset> asset = assetRegistry.GetAsset<MeshAsset>("Lvl_Blockout_NavMesh.fbx");
	//	component->SetMesh(asset);
	//}

	if (EnableCollisionDemo)
	{
		// Debug code: build a small test scene to watch overlap events, separate from normal gameplay.
		ColliderComponent* movingSphere = SpawnDebugCollisionSphere(aWorld, "CollisionDemoMoving", 40.0f, 0);
		movingSphere->SetMask(1u << 1);

		ColliderComponent* fixedSphere = SpawnDebugCollisionSphere(aWorld, "CollisionDemoSphere", 60.0f, 1);
		ColliderComponent* filteredSphere = SpawnDebugCollisionSphere(aWorld, "CollisionDemoFiltered", 50.0f, 2);

		Actor* boxActor = aWorld.SpawnActor("CollisionDemoBox");
		boxActor->GetTransform().SetLocalRotationDegrees(35.0f, 0, 0);
		auto* box = boxActor->AddComponent<ColliderComponent>("Collider");
		box->SetBoxHalfExtents({60, 60, 60});
		box->SetLayer(1);
		box->SetMask(1u << 0);
		aWorld.SpawnActor("CollisionDemo")->AddComponent<CollisionDemoComponent>("Demo",
			movingSphere->GetColliderId(), fixedSphere->GetColliderId(), box->GetColliderId(), filteredSphere->GetColliderId());
	}

	{ // TODO : Temporary place for debug animation
		AnimationManager::DebugData& data = ServiceLocator::GetInstance().GetAnimationManager().GetDebugData();

		if (!data.Enabled)
		{
			return;
		}

		Actor* playerBro = aWorld.SpawnActor("A_Debug");
		playerBro->GetTransform().SetLocalRotationDegrees({ 0, 0, 0 });
		playerBro->GetTransform().SetWorldPosition({ data.Position.x, data.Position.y, data.Position.z });

		SkeletalMeshComponent* component = playerBro->AddComponent<SkeletalMeshComponent>();
		AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();
		std::shared_ptr<MeshAsset> meshAsset = assetRegistry.GetAsset<MeshAsset>(data.Mesh);
		component->SetMesh(meshAsset);

		AnimatorComponent* animatorBro = playerBro->AddComponent<AnimatorComponent>();
		animatorBro->SetMeshComponent(component);
		animatorBro->SetTree(data.Tree);

		animatorBro->GetTree().SetBool("IsIdle", true);

		CommonUtilities::InputMapper& map = ServiceLocator::GetInstance().GetInputMapper();

		EKeyCode key = EKeyCode::NUMPAD0;
		for (AnimationManager::Variable& var : data.Variables)
		{
			map.BindActionToInputCode(var.Name, key);

			map.AddEventListener(var.Name, [var, animatorBro](const CommonUtilities::InputEvent event)
				{
					if (!event.inputData.isPressed)
					{
						return;
					}
					if (var.Type == "Bool")
					{
						animatorBro->GetTree().SetBool(var.Name, !animatorBro->GetTree().GetAnimationVariable(var.Name).IsActive);
					}
					else
					{
						animatorBro->GetTree().SetTrigger(var.Name);
					}
				});

			if (key == EKeyCode::NUMPAD9)
			{
				break;
			}
			key = EKeyCode(static_cast<int>(key) + 1);
		}
	} //Dont put anything below this line
}
