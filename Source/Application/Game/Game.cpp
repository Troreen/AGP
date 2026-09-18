#include "Game.h"
#include "GameComponents.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/World/World.h"
#include "GameLog.h"

DEFINE_LOG_CATEGORY(LogGame);

namespace
{
	constexpr float BackgroundMusicVolume = 0.35f;
	constexpr char SourceChestName[] = "Chest_Opaque";
	constexpr char RuntimeChestName[] = "RuntimeSpinningChest";
	constexpr char RuntimeChestMeshName[] = "ChestMesh";
	constexpr char RuntimeChildMeshName[] = "OrbitingChildChest";
	constexpr char RuntimeChildSpinName[] = "ChildSpin";
	const CU::Vector3f RuntimeChestPosition{-250.0f, 0.0f, 0.0f};
	const CU::Vector3f RuntimeChildPosition{220.0f, 0.0f, 0.0f};
	const CU::Vector3f RuntimeChildScale{0.4f, 0.4f, 0.4f};

	bool CopyMeshAppearance(const StaticMeshComponent& source, StaticMeshComponent& destination)
	{
		destination.SetMesh(source.GetMesh());
		if (!destination.HasMesh())
		{
			return false;
		}

		destination.SetSourceAssetIdentity(source.GetSourceMeshName(), source.GetSourceContentPath());
		for (unsigned materialIndex = 0; materialIndex < source.GetMaterialCount(); ++materialIndex)
		{
			const MaterialAsset material = source.GetMaterial(materialIndex);
			if (material)
			{
				destination.SetMaterial(materialIndex, material);
			}
		}
		return true;
	}

	StaticMeshComponent* FindSourceChestMesh(World& world)
	{
		Actor* sourceChest = world.FindActor(SourceChestName);
		return sourceChest ? sourceChest->GetComponent<StaticMeshComponent>() : nullptr;
	}

	void SpawnRuntimeChest(World& world)
	{
		StaticMeshComponent* sourceMesh = FindSourceChestMesh(world);
		if (!sourceMesh)
		{
			GAMELOG(Warning, "Cannot spawn the runtime chest: '{}' has no static mesh", SourceChestName);
			return;
		}

		Actor* chest = world.SpawnActor(RuntimeChestName);
		chest->GetTransform().SetLocalPosition(RuntimeChestPosition);
		StaticMeshComponent* chestMesh = chest->AddComponent<StaticMeshComponent>(RuntimeChestMeshName);
		CopyMeshAppearance(*sourceMesh, *chestMesh);
		chest->AddComponent<SpinComponent>("Spin");
		GAMELOG(Log, "Spawned '{}'; press F8 to attach its child chest", RuntimeChestName);
	}

	void ToggleRuntimeChest(World& world)
	{
		if (Actor* chest = world.FindActor(RuntimeChestName))
		{
			chest->Destroy();
			GAMELOG(Log, "Destroyed '{}'", RuntimeChestName);
			return;
		}
		SpawnRuntimeChest(world);
	}

	void AttachRuntimeChild(World& world)
	{
		Actor* chest = world.FindActor(RuntimeChestName);
		if (!chest)
		{
			GAMELOG(Warning, "Press F7 to spawn '{}' before attaching its child", RuntimeChestName);
			return;
		}
		if (chest->FindComponent(RuntimeChildMeshName))
		{
			GAMELOG(Log, "'{}' already has its child chest", RuntimeChestName);
			return;
		}

		StaticMeshComponent* parentMesh = chest->GetComponent<StaticMeshComponent>();
		if (!parentMesh)
		{
			GAMELOG(Warning, "Cannot attach a child: '{}' has no static mesh", RuntimeChestName);
			return;
		}

		StaticMeshComponent* childMesh = chest->AddComponent<StaticMeshComponent>(RuntimeChildMeshName);
		if (!CopyMeshAppearance(*parentMesh, *childMesh))
		{
			childMesh->Destroy();
			GAMELOG(Warning, "Cannot attach a child: the source mesh is unavailable");
			return;
		}
		childMesh->GetTransform().SetLocalPosition(RuntimeChildPosition);
		childMesh->GetTransform().SetLocalScale(RuntimeChildScale);
		SpinComponent* childSpin = chest->AddComponent<SpinComponent>(RuntimeChildSpinName);
		childSpin->SetTargetComponentName(RuntimeChildMeshName);
		GAMELOG(Log, "Attached a self-spinning child chest to '{}'; the parent rotation drives its orbit", RuntimeChestName);
	}
}

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize(GameContext& context)
{
	InputSystem& input = context.GetInputSystem();
	myInputSubscriptions.push_back(input.Subscribe(InputActions::Quit, [&context](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started)
		{
			context.RequestQuit();
		}
	}));
	myInputSubscriptions.push_back(input.Subscribe(InputActions::ReloadScene, [&context](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started)
		{
			context.ReloadScene();
		}
	}));
	myInputSubscriptions.push_back(input.Subscribe(InputActions::SpawnDemo, [&context](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started)
		{
			ToggleRuntimeChest(context.GetWorld());
		}
	}));
	myInputSubscriptions.push_back(input.Subscribe(InputActions::AttachDemoChild, [&context](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started)
		{
			AttachRuntimeChild(context.GetWorld());
		}
	}));
	context.LoadScene("ChestMaterials");
	AudioManager& audio = ServiceLocator::GetInstance().GetAudioManager();
	audio.SetBusVolume(BusID::eMusic, BackgroundMusicVolume);
	audio.PlayMusic(SoundID::eMainTheme, true);
	GAMELOG(Log, "Game ready: F7 toggles a spinning chest, F8 attaches its child, R toggles spinning");
}

void Game::Update(GameContext&, float) {}

void Game::Shutdown(GameContext& context)
{
	ServiceLocator::GetInstance().GetAudioManager().StopMusic(SoundID::eMainTheme, false);
	myInputSubscriptions.clear();
	context.GetWorld().SetActiveCamera(nullptr);
}

void Game::ConfigureWorld(World& world)
{
	// Code-configured behavior example: exported scene data supplies the Actor
	// and its two PointLightComponents, while game code attaches reusable logic.
	// The offset SceneComponent inherits this Actor rotation and orbits the center.
	if (Actor* doubleLight = world.FindActor("DoubleLight"))
	{
		doubleLight->AddComponent<SpinComponent>("Spin");
	}
}
