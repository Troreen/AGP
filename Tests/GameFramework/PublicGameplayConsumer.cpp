#include <InputMapper.h>
#include <GameFramework/ServiceLocator.h>
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
	std::vector<unsigned> myListeners;
	void ConfigureWorld(World& world) override
	{
		world.SpawnActor("Configured")->AddComponent<Move>();
	}

	void Initialize(GameContext& game) override
	{
		auto& input = *ServiceLocator::GetInstance().GetInputMapper();
		input.BindActionToInputCode("ReloadScene", EKeyCode::F4);

		myListeners.push_back(input.AddEventListener("ReloadScene", [&game](const CommonUtilities::InputEvent& event)
		{
			if (event.inputData.isPressed) game.ReloadScene();
		}));
		game.GetWorld().SpawnActor("Player")->AddComponent<Move>();
	}

	void Shutdown(GameContext&) override
	{
		for (unsigned id : myListeners) ServiceLocator::GetInstance().GetInputMapper()->RemoveEventListener(id);
		myListeners.clear();
	}
	void Update(GameContext&, float) override {}
};
