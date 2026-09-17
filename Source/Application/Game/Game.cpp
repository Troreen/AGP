#include "Game.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameComponents.h"
#include "GameLog.h"

DEFINE_LOG_CATEGORY(LogGame);

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize(GameContext& context)
{
	context.LoadScene("Game");
	GAMELOG(
	    Log,
	    "Game ready: RMB + WASD/Space/Ctrl camera, R chest spin, numpad 0-3 animation, 7-9 lights, Shift+7-9 place lights, F5 reload, F7 spawn/destroy chest, Esc quit");
}

void Game::Update(GameContext& context, float)
{
	if (context.GetInput().IsKeyPressed(Keys::ESCAPE))
	{
		context.RequestQuit();
	}
	if (context.GetInput().IsKeyPressed(Keys::F5))
	{
		context.ReloadScene();
	}
	if (context.GetInput().IsKeyPressed(Keys::F7))
	{
		auto& world = context.GetWorld();
		if (auto* demo = world.FindActor("Spawned Chest"))
		{
			demo->Destroy();
		}
		else if (auto* source = world.FindActor("SM_Chest Actor"))
		{
			auto* original = source->GetComponent<StaticMeshComponent>();
			if (!original)
			{
				return;
			}
			auto* demo = world.SpawnActor("Spawned Chest");
			demo->GetTransform().SetLocalPosition({350, 0, 285});
			demo->AddComponent<SpinComponent>("Spin");
			auto* mesh = demo->AddComponent<StaticMeshComponent>("Mesh", original->GetMesh());
			for (unsigned slot = 0; slot < original->GetMaterialCount(); ++slot)
			{
				mesh->SetMaterial(slot, original->GetMaterial(slot));
			}
		}
	}
}

void Game::Shutdown(GameContext& context)
{
	context.GetWorld().SetActiveCamera(nullptr);
}

void Game::RegisterComponents(ComponentRegistry& registry)
{
	registry.Register<CameraControlsComponent>("CameraControls");
	registry.Register<AnimationControlsComponent>("AnimationControls");
	registry.Register<SpinComponent>("Spin");
	registry.Register<LightControlsComponent>("LightControls", [](LightControlsComponent& c, const SceneReader& fields)
	{
		c.CameraName = fields.OptionalString("camera");
		c.DirectionalName = fields.OptionalString("directional");
		c.PointName = fields.OptionalString("point");
		c.SpotName = fields.OptionalString("spot");
	});
}
