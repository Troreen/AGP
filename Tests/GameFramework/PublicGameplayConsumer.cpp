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
	std::vector<InputSubscription> mySubscriptions;
	void ConfigureWorld(World& world) override
	{
		world.SpawnActor("Configured")->AddComponent<Move>();
	}

	void Initialize(GameContext& game) override
	{
		mySubscriptions.push_back(game.GetInputSystem().Subscribe(InputActions::Quit, [&game](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Started) game.RequestQuit();
		}));
		mySubscriptions.push_back(game.GetInputSystem().Subscribe(InputActions::ReloadScene, [&game](const InputActionEvent& event)
		{
			if (event.Phase == InputActionPhase::Started) game.ReloadScene();
		}));
		game.GetWorld().SpawnActor("Player")->AddComponent<Move>();
	}

	void Update(GameContext&, float) override {}
};
