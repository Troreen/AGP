#include "Game.h"
#include "Runtime/GameApplication.h"
#include "GameLog.h"
#include <GameFramework/AudioManager.h>
#include <GameFramework/AssetHandling/AssetRegistry.h>
#include <GameFramework/AssetHandling/MaterialAsset.h>
#include <GameFramework/AssetHandling/MeshAsset.h>
#include <GameFramework/AssetHandling/ParticleSystemAsset.h>
#include <GameFramework/AssetHandling/TextureAsset.h>
#include <GameFramework/Components/SkeletalMeshComponent.h>
#include <GameFramework/Components/StaticMeshComponent.h>
#include <GameFramework/Components/SpriteComponent.h>
#include <GameFramework/Components/ParticleComponent.h>
#include <GameFramework/Scenes/SceneManager.h>
#include <GameFramework/ServiceLocator.h>
#include <GameFramework/World/World.h>
#include "InputMapper.h"
#include "GameFramework/Components/ColliderComponent.h"

#include <memory>

DEFINE_LOG_CATEGORY(LogGame);

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize()
{
	CommonUtilities::InputMapper& input = ServiceLocator::GetInstance().GetInputMapper();

	myInputListenerIDs.push_back(input.AddEventListener("ReloadScene", [](const CommonUtilities::InputEvent& anEvent)
		{
			if (anEvent.inputData.isPressed)
			{
				ServiceLocator::GetInstance().GetSceneManager().ReloadCurrentScene();
			}
		}));

	ServiceLocator::GetInstance().GetSceneManager().OnLoadScene().Subscribe([this]() { ConfigureWorld(); });
	ServiceLocator::GetInstance().GetAudioManager().PlaySFX(eIntroSwell);

	GAMELOG(Log, "Game ready: F4 reloads the current scene, ESC quits the game.");
}

void Game::Update([[maybe_unused]] World& aWorld, [[maybe_unused]] float aDeltaTime)
{
	const std::shared_ptr<SceneAsset>& currentScene = ServiceLocator::GetInstance().GetSceneManager().GetCurrentScene();

	if (currentScene != nullptr && currentScene->GetSceneName() == "Lvl_Blistering_Tundra_Level")
	{
		ServiceLocator::GetInstance().GetAudioManager().PlayMusic(eBlizzardAmbience, true);
	}
	else
	{
		ServiceLocator::GetInstance().GetAudioManager().StopMusic(eBlizzardAmbience, false);
	}

	if (currentScene != nullptr && currentScene->GetSceneName() == "Lvl_Forsaken_Castle_Level")
	{
		ServiceLocator::GetInstance().GetAudioManager().PlayMusic(eCastleAmbienceTwo, true);
	}
	else
	{
		ServiceLocator::GetInstance().GetAudioManager().StopMusic(eCastleAmbienceTwo, false);
	}

	// The game chooses to listen from the active view, including the debug camera.
	// Commenting this out becasue it spams the log with info about listener position
	//if (const CameraComponent* camera = aWorld.GetActiveCamera())
	//{
	//	ServiceLocator::GetInstance().GetAudioManager().SetAudioListener(camera->GetWorldPosition(), camera->GetWorldForward(), camera->GetWorldUp());
	//}
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

void Game::ConfigureWorld()
{
	// TODO: remember to turn this off wheen handing in bozos
	ColliderComponent::SetAllDebugDrawEnabled(true);
	World& world = *ServiceLocator::GetInstance().GetSceneManager().GetLoadedWorld();

	{
		Actor* actor = world.SpawnActor("GlowCube");
		StaticMeshComponent* component = actor->AddComponent<StaticMeshComponent>();

		MaterialDescription tempMatDesc{};
		tempMatDesc.Domain = MaterialDomain::Surface;
		tempMatDesc.ShadingModel = ShadingModel::Lit;
		tempMatDesc.BlendMode = BlendMode::Opaque;
		tempMatDesc.Name = "Glow boi";

		std::shared_ptr<Material> tempMat = std::make_shared<Material>();
		GraphicsEngine::Get().CreateMaterial(tempMatDesc, *tempMat);

		std::shared_ptr<Texture> allEmissionColor = std::make_shared<Texture>();
		GraphicsEngine::Get().CreateColorTexture("WOW all emission", { 255, 0, 0, 0 }, *allEmissionColor);
		tempMat->SetTexture(3, allEmissionColor);

		std::shared_ptr<MaterialAsset> tempMatAss = std::make_shared<MaterialAsset>(tempMat);

		AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();
		assetRegistry.Pin("Glow boi", tempMatAss);

		component->SetMesh(assetRegistry.GetAsset<MeshAsset>("SM_Cube.fbx"));
		component->SetMaterial(0, tempMatAss);

		float currentAngle = 0;
		constexpr size_t numberOfCubes = 10;
		constexpr float angleIncrease = (2.0f * static_cast<float>(std::numbers::pi)) / numberOfCubes;
		constexpr float rotationIncrease = (360.0f / numberOfCubes) * CU::Math::DEGREES_TO_RADIANS<float>;
		constexpr float circleRadius = 50.0f;

		for (size_t i = 0; i < numberOfCubes; i++)
		{
			const float x = std::cos(currentAngle);
			const float z = std::sin(currentAngle);

			const CU::Vector3f position = CU::Vector3f(x * circleRadius, 1000, z * circleRadius);

			CU::Matrix4f worldTransform = CU::Matrix4f::FromRotation(0, 0, i * rotationIncrease);
			worldTransform *= CU::Matrix4f::FromTranslation(position.x, position.y, position.z);
			component->AddInstance(InstanceData{
				.World = worldTransform,
				.WorldInvT = worldTransform.GetInverse().GetTranspose()
			});

			currentAngle += angleIncrease;
		}

		actor->GetTransform().SetWorldPosition({ 0, 1000, 0 });
	}

	{
		Actor* actor = world.SpawnActor("Particle");
		actor->GetTransform().SetWorldPosition({ 0, 300, 0 });
		ParticleComponent* component = actor->AddComponent<ParticleComponent>();

		MaterialDescription particleMatDesc;
		particleMatDesc.Domain = MaterialDomain::Particles;
		particleMatDesc.ShadingModel = ShadingModel::Unlit;
		particleMatDesc.BlendMode = BlendMode::Additive;
		particleMatDesc.Name = "TestParticles";

		std::shared_ptr<Material> particleMat = std::make_shared<Material>();
		GraphicsEngine::Get().CreateMaterial(particleMatDesc, *particleMat);
		std::shared_ptr<MaterialAsset> mat = std::make_shared<MaterialAsset>(particleMat);

		AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();
		assetRegistry.Pin("TestParticles", mat);

		std::shared_ptr<ParticleSystemAsset> asset = assetRegistry.GetAsset<ParticleSystemAsset>("Test.particles");
		component->SetParticleSystem(asset);
	}

	{
		Actor* actor = world.SpawnActor("TGALOGO");
		SpriteComponent* component = actor->AddComponent<SpriteComponent>();

		AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();

		std::shared_ptr<TextureAsset> asset = assetRegistry.GetAsset<TextureAsset>("TgaLogo.dds");
		component->SetSprite(asset);
		component->SetSize({ 2048.0f * 0.5f, 512.0f * 0.5f });
		component->SetPivot({ 1, 1 });
		component->SetPosition({ 0.0f, 0.0f });
		component->SetAnchor({ 1.0f, 1.0f });
	}

	//{
	//	Actor* actor = world.SpawnActor("NavmeshTest");
	//	SkeletalMeshComponent* component = actor->AddComponent<SkeletalMeshComponent>();
	//	actor->GetTransform().SetWorldPosition({ -100, 100, 0 });

	//	AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();

	//	std::shared_ptr<MeshAsset> asset = assetRegistry.GetAsset<MeshAsset>("Lvl_Blockout_NavMesh.fbx");
	//	component->SetMesh(asset);
	//}
}
