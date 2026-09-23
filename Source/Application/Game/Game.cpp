#include "Game.h"
#include "GameApplication.h"
#include "GameLog.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/World/World.h"
#include "InputMapper.h"

#include <filesystem>
#include <memory>

DEFINE_LOG_CATEGORY(LogGame);


constexpr float BackgroundMusicVolume = 0.35f;


Game::Game() = default;
Game::~Game() = default;

void Game::Initialize(GameApplication& anApplication)
{
	myMeshLibrary.Initialize(ServiceLocator::GetInstance().GetAssetRegistry().GetContentRoot());
	auto& input = *ServiceLocator::GetInstance().GetInputMapper();
	input.BindActionToInputCode("ReloadScene", EKeyCode::F4);


	myInputListenerIDs.push_back(input.AddEventListener("ReloadScene", [&anApplication](const CommonUtilities::InputEvent& anEvent)
	{
		if (anEvent.inputData.isPressed)
		{
			anApplication.ReloadCurrentScene();
		}
	}));
	
	anApplication.RequestSceneLoad(SceneId::Blockout);
	AudioManager& audio = ServiceLocator::GetInstance().GetAudioManager();
	audio.SetBusVolume(BusID::eMusic, BackgroundMusicVolume);
	audio.PlayMusic(SoundID::eMainTheme, true); // TODO: make man breathe more often this is not enough wtf smh b-word
	GAMELOG(Log, "Game ready: F4 reloads the current scene, ESC quits the game.");
}

void Game::Update(World& aWorld, float)
{
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
	ServiceLocator::GetInstance().GetAudioManager().StopMusic(SoundID::eMainTheme, false);
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
		//AnimatorComponent* animator = bro->AddComponent<AnimatorComponent>();

		const std::filesystem::path& contentRoot = ServiceLocator::GetInstance().GetAssetRegistry().GetContentRoot();
		myMeshLibrary.LoadFBXMesh(contentRoot / "Meshes/Characters/TGA_Bro/SK_C_TGA_Bro.fbx");
		myMeshLibrary.LoadFBXAnimation("SK_C_TGA_Bro", "Idle",
			contentRoot / "Animations/Characters/TGA_Bro/Idle/A_C_TGA_Bro_Idle_Brething.fbx");

		std::shared_ptr<Mesh> mesh = myMeshLibrary.GetMesh("SK_C_TGA_Bro");

		std::shared_ptr<MeshAsset> meshAsset = std::make_shared<MeshAsset>(mesh);
		
		component->SetMesh(meshAsset);
		component->PlayAnimation("Idle", true);
		bigcomponent->SetMesh(meshAsset);
		bigcomponent->PlayAnimation("Idle", true);
	}
}
