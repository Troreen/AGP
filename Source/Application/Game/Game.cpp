#include "Game.h"
#include "GameComponents.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/World/World.h"
#include "GameLog.h"


#include "../../Engine/GameFramework/Components/SkeletalMeshComponent.h"
#include "../../Engine/GraphicsEngine/Objects/Mesh.h"
#include <MeshLibrary.h>

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

		for (unsigned materialIndex = 0; materialIndex < source.GetMaterialCount(); ++materialIndex)
		{
			const MaterialHandle material = source.GetMaterial(materialIndex);
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
	// TODO!: input handling having to be done like this doesnt feel right
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
	context.LoadScene(SceneType::Blockout); // TODO: make this an enum or smn wtf
	AudioManager& audio = ServiceLocator::GetInstance().GetAudioManager();
	audio.SetBusVolume(BusID::eMusic, BackgroundMusicVolume);
	audio.PlayMusic(SoundID::eMainTheme, true); // TODO: make man breathe more often this is not enough wtf smh b-word
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

	{
		Actor* bro = world.SpawnActor("TGE_BRO");
		bro->GetTransform().SetLocalRotationDegrees({ 180, 0, 0 });
		bro->GetTransform().SetWorldPosition({ 0, 200, 0 });
			
		SkeletalMeshComponent* component = bro->AddComponent<SkeletalMeshComponent>();

		MeshLibrary meshLib;

		std::filesystem::path path = std::filesystem::current_path();
		path = path / "..\\..\\..\\";

		meshLib.LoadFBXMesh(path / "Content\\Meshes\\Characters\\TGA_Bro\\SK_C_TGA_Bro.fbx");
		meshLib.LoadFBXAnimation("SK_C_TGA_Bro","Idle",path / "Content\\Animations\\Characters\\TGA_Bro\\Idle\\A_C_TGA_Bro_Idle_Brething.fbx");

		std::shared_ptr<Mesh> mesh = meshLib.GetMesh("SK_C_TGA_Bro");

		component->SetMesh_DO_NOT_USE(mesh);
		component->PlayAnimation("Idle", true);
	}

	// Code-configured behavior example: exported scene data supplies the Actor
	// and its two PointLightComponents, while game code attaches reusable logic.
	// The offset SceneComponent inherits this Actor rotation and orbits the center.
	if (Actor* doubleLight = world.FindActor("DoubleLight"))
	{
		doubleLight->AddComponent<SpinComponent>("Spin");
	}
}
