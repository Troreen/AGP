#include "Game.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameLog.h"

DEFINE_LOG_CATEGORY(LogGame);

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize(GameContext& context)
{
	auto& input = context.GetInputSystem();
	myInputSubscriptions.push_back(input.Subscribe(InputActions::Quit, [&context](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started) context.RequestQuit();
	}));
	myInputSubscriptions.push_back(input.Subscribe(InputActions::ReloadScene, [&context](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started) context.ReloadScene();
	}));
	context.LoadScene("Game");
	GAMELOG(Log, "Game ready: action input enabled; F1 debug camera, F5 reload, Esc quit");
}

void Game::Update(GameContext&, float) {}

void Game::Shutdown(GameContext& context)
{
	myInputSubscriptions.clear();
	context.GetWorld().SetActiveCamera(nullptr);
}

void Game::ConfigureWorld(World&) {}
