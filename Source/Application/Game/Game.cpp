#include "Game.h"
#include "GameApplication.h"
#include "GameLog.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/AnimationAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/World/World.h"
#include "InputMapper.h"

#include <memory>

DEFINE_LOG_CATEGORY(LogGame);

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize(GameApplication& anApplication)
{
	auto& input = *ServiceLocator::GetInstance().GetInputMapper();

	myInputListenerIDs.push_back(input.AddEventListener("ReloadScene", [&anApplication](const CommonUtilities::InputEvent& anEvent)
	{
		if (anEvent.inputData.isPressed)
		{
			anApplication.ReloadCurrentScene();
		}
	}));
	
	AudioManager& audio = ServiceLocator::GetInstance().GetAudioManager();
	audio.PlayMusic(SoundID::eMainTheme, true); // Todo: Get Viggo Mortensen's Signature
	GAMELOG(Log, "Game ready: F4 reloads the current scene, ESC quits the game.");
}

void Game::Update([[maybe_unused]] World& aWorld, [[maybe_unused]] float aDeltaTime)
{
	if (!ServiceLocator::GetInstance().GetAudioManager().IsEventPlaying(eMainTheme))
	{
		ServiceLocator::GetInstance().GetAudioManager().PlayMusic(eMainTheme);
	}
}

void Game::DrawDebugUI()
{
	// Add game-specific ImGui windows here. Called between NewFrame and Render.
}

void Game::Shutdown()
{
	if (auto* input = ServiceLocator::GetInstance().GetInputMapper())
	{
		for (unsigned id : myInputListenerIDs) 
		{
			input->RemoveEventListener(id);
		}
	}

	myInputListenerIDs.clear();
}

void Game::ConfigureWorld(World& aWorld)
{
	{
		Actor* big = aWorld.SpawnActor("TGE_BIG");
		big->GetTransform().SetLocalRotationDegrees({ 180, 0, 0 });
		big->GetTransform().SetWorldPosition({ 0, 200, 0 });
		big->GetTransform().SetLocalScale({ 10, 10, 10 });

		Actor* bro = aWorld.SpawnActor("TGE_BRO");
		bro->GetTransform().SetLocalRotationDegrees({ 180, 0, 0 });
		bro->GetTransform().SetWorldPosition({ 0, 900, -450 });
			
		SkeletalMeshComponent* component = bro->AddComponent<SkeletalMeshComponent>();
		SkeletalMeshComponent* bigcomponent = big->AddComponent<SkeletalMeshComponent>();
		AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();

		std::shared_ptr<MeshAsset> meshAsset = assetRegistry.GetAsset<MeshAsset>("SK_C_TGA_Bro.fbx");
		std::shared_ptr<AnimationAsset> animationAsset = assetRegistry.GetAsset<AnimationAsset>("A_C_TGA_Bro_Idle_Brething");
		
		component->SetMesh(meshAsset);
		component->AddAnimation("Idle", animationAsset);
		component->PlayAnimation("Idle", true);
		bigcomponent->SetMesh(meshAsset);
		bigcomponent->AddAnimation("Idle", animationAsset);
		bigcomponent->PlayAnimation("Idle", true);
	}
}
