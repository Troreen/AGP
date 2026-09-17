#include <GameFramework/Runtime/IGame.h>
#include <GameFramework/Runtime/GameContext.h>
#include <GameFramework/Scenes/ComponentRegistry.h>

class Move final : public Component
{
public:
	float Speed = 100;

	void Update(float dt) override
	{
		auto& transform = GetOwner()->GetTransform();
		transform.SetLocalPosition(transform.GetLocalPosition() + CommonUtilities::Vector3f{0, 0, Speed * dt});
	}
};

class Example final : public IGame
{
	void RegisterComponents(ComponentRegistry& registry) override
	{
		registry.Register<Move>("Move", [](Move& move, const SceneReader& fields)
		{
			move.Speed = fields.OptionalFloat("speed", 100);
		});
	}

	void Initialize(GameContext& game) override
	{
		game.GetWorld().SpawnActor("Player")->AddComponent<Move>();
	}

	void Update(GameContext& game, float) override
	{
		if (game.GetInput().IsKeyPressed(Keys::ESCAPE))
		{
			game.RequestQuit();
		}
		if (game.GetInput().IsKeyPressed(Keys::F5))
		{
			game.ReloadScene();
		}
	}
};
