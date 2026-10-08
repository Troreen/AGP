#include "Game.h"
#include "GameApplication.h"
#include "GameLog.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/AnimationAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/AnimatorComponent.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/World/World.h"
#include "InputMapper.h"

#include <memory>

DEFINE_LOG_CATEGORY(LogGame);

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

	ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eIntroSwell);

	GAMELOG(Log, "Game ready: F4 reloads the current scene, ESC quits the game.");
}

void Game::Update([[maybe_unused]] World& aWorld, [[maybe_unused]] float aDeltaTime)
{
	if (!ServiceLocator::GetInstance().GetAudioManager().IsEventPlaying(eBlizzardAmbience))
	{
		ServiceLocator::GetInstance().GetAudioManager().PlayMusic(eBlizzardAmbience);
	}
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
	{
		Actor* playerBro = aWorld.SpawnActor("PLAYAH_BRO");
		playerBro->GetTransform().SetLocalRotationDegrees({ 180, 0, 0 });
		playerBro->GetTransform().SetWorldPosition({ 0, -15, 0 });

		SkeletalMeshComponent* component = playerBro->AddComponent<SkeletalMeshComponent>();
		AnimatorComponent* animatorBro = playerBro->AddComponent<AnimatorComponent>();
		animatorBro->SetMeshComponent(component);
		animatorBro->Init("PlayerTree");
		AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();

		std::shared_ptr<MeshAsset> meshAsset = assetRegistry.GetAsset<MeshAsset>("SK_player.fbx");

		component->SetMesh(meshAsset);
		animatorBro->GetTree().SetBool("IsIdle", true);
		animatorBro->GetTree().PlayState("IdleState");
	}
}
