#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>
#include "GameFramework/Runtime/GameApplication.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "Game.h"
#include "GameScene.h"
#include "GameComponents.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include <chrono>
#include <iostream>

void Check(bool value, const char* message)
{
	if (!value)
	{
		throw std::runtime_error(message);
	}
}

// Actual content: rejected replacement, reload, and empty scene presentation.
class SampleGame final : public IGame
{
public:
	Game Sample;
	int Loads = 0, Frames = 0, Shutdowns = 0;
	bool InvalidRequested = false, Failed = false, ReloadRequested = false, EmptyRequested = false;
	std::chrono::steady_clock::time_point Deadline;

	void RegisterComponents(ComponentRegistry& registry) override
	{
		Sample.RegisterComponents(registry);
	}

	void Initialize(GameContext& context) override
	{
		Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
		Sample.Initialize(context);
	}

	void OnSceneLoaded(GameContext& context, const std::string& name) override
	{
		++Loads;
		Frames = 0;
		auto& world = context.GetWorld();
		if (name == "Empty")
		{
			Check(Loads == 3 && !world.FindActor("Camera Actor") && !world.GetActiveCamera(), "Empty replacement retained old objects");
			return;
		}
		Check(name == "Game", "Wrong scene");
		auto* camera = world.FindActor("Camera Actor");
		Check(camera && world.GetActiveCamera() == camera->GetComponent<CameraComponent>(), "Actual camera missing");
		auto* chest = world.FindActor("SM_Chest Actor");
		Check(chest && chest->GetComponent<StaticMeshComponent>() && chest->GetComponent<SpinComponent>()->HasBegunPlay(),
		      "Actual chest lifecycle");
		auto* character = world.FindActor("TGA Bro Actor");
		Check(character && character->GetComponent<SkeletalMeshComponent>(), "Actual skeletal mesh missing");
		const auto expected = (CommonUtilities::Vector3f{25, 0, 260} - camera->GetTransform().GetWorldPosition()).GetNormalized();
		Check((expected - camera->GetTransform().GetLocalForward()).Length() < .0001f, "Authored camera aim changed");
		if (Loads == 1)
		{
			world.SpawnActor("Old-scene-only");
		}
		else
		{
			Check(!world.FindActor("Old-scene-only"), "Reload retained an old Actor");
		}
	}

	void OnSceneLoadFailed(GameContext& context, const std::string& name, const std::string& error) override
	{
		Check(name == "Invalid" && error.find("MissingType") != std::string::npos, "Failure lost useful diagnostics");
		Check(context.GetSceneName() == "Game" && context.GetWorld().FindActor("Old-scene-only") && context.GetWorld().GetActiveCamera(),
		      "Failed construction damaged the live scene");
		Failed = true;
	}

	void Update(GameContext& context, float delta) override
	{
		Check(std::chrono::steady_clock::now() < Deadline, "Runtime timed out");
		Sample.Update(context, delta);
		++Frames;
		const auto stats = GraphicsEngine::Get().GetLastRenderStats();
		const bool rendered =
		    stats.TotalRenderItems >= 5 && stats.VisibleRenderItems > 0 && stats.BlendedRenderItems > 0 && stats.TotalLights == 3;
		if (Loads == 1 && Frames >= 5 && rendered && !InvalidRequested)
		{
			InvalidRequested = true;
			context.LoadScene("Invalid");
		}
		if (Failed && !ReloadRequested)
		{
			ReloadRequested = true;
			Check(context.ReloadScene(), "Reload refused");
		}
		if (Loads == 2 && Frames >= 5 && rendered && !EmptyRequested)
		{
			EmptyRequested = true;
			context.LoadScene("Empty");
		}
		if (Loads == 3 && Frames >= 2 && stats.TotalRenderItems == 0 && stats.TotalLights == 0)
		{
			context.RequestQuit();
		}
	}

	void Shutdown(GameContext& context) override
	{
		++Shutdowns;
		Sample.Shutdown(context);
	}
};

struct LifetimeCounts
{
	int Begins = 0, Updates = 0, Ends = 0, Destroyed = 0;
};

class Lifetime final : public Component
{
public:
	LifetimeCounts* Counts = nullptr;
	std::string Failure;

	~Lifetime() override
	{
		if (Counts)
		{
			++Counts->Destroyed;
		}
	}

	void BeginPlay() override
	{
		++Counts->Begins;
		if (Failure == "begin-failure")
		{
			throw std::runtime_error("Expected BeginPlay failure");
		}
	}

	void Update(float) override
	{
		++Counts->Updates;
		if (Failure == "component-failure")
		{
			throw std::runtime_error("Expected component Update failure");
		}
	}

	void EndPlay() noexcept override
	{
		++Counts->Ends;
	}
};

class FailureGame final : public IGame
{
public:
	std::string Scenario;
	LifetimeCounts Counts;
	int Shutdowns = 0, Failures = 0, Updates = 0;

	void RegisterComponents(ComponentRegistry& registry) override
	{
		registry.Register<Lifetime>("Lifetime", [this](Lifetime& c, const SceneReader&)
		{
			c.Counts = &Counts;
			c.Failure = Scenario;
		});
	}

	void Initialize(GameContext& context) override
	{
		if (Scenario == "initialize-failure")
		{
			throw std::runtime_error("Expected Initialize failure");
		}
		context.LoadScene("Fixture");
	}

	void OnSceneLoadFailed(GameContext&, const std::string&, const std::string&) override
	{
		++Failures;
	}

	void Update(GameContext& context, float) override
	{
		if (Scenario == "update-failure")
		{
			throw std::runtime_error("Expected game Update failure");
		}
		if (++Updates > 2)
		{
			context.RequestQuit();
		}
	}

	void Shutdown(GameContext& context) override
	{
		++Shutdowns;
		Check(!context.LoadScene("During shutdown"), "Shutdown accepted a scene request");
		if (Scenario == "shutdown-failure")
		{
			throw std::runtime_error("Expected Shutdown failure");
		}
	}
};

int RunCameraControlsTests();

int main(int argc, char** argv)
{
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	const std::string scenario = argc > 1 ? argv[1] : "sample";
	if (scenario == "camera-controls")
	{
		return RunCameraControlsTests();
	}
	GameApplication::Config config;
	config.ShowWindow = false;
	config.Width = 640;
	config.Height = 360;
	config.ContentRoot = std::filesystem::current_path() / "Content";
	try
	{
		if (scenario == "sample")
		{
			SampleGame game;
			GameScene source;
			GameApplication{}.Run(game, config, [&](const std::string& name, SceneLoadContext& context)
			{
				if (name == "Empty")
				{
					return SceneData{};
				}
				if (name == "Invalid")
				{
					SceneData data;
					ActorRecord actor;
					actor.Name = "Broken";
					ComponentRecord c;
					c.Name = "Broken";
					c.Type = "MissingType";
					actor.Components.push_back(c);
					data.Actors.push_back(actor);
					return data;
				}
				return source.Load(name, context);
			});
			Check(game.Loads == 3 && game.Failed && game.Shutdowns == 1, "Actual sample lifecycle incomplete");
		}
		else
		{
			FailureGame game;
			game.Scenario = scenario;
			bool caught = false;
			try
			{
				GameApplication{}.Run(game, config, [&](const std::string&, SceneLoadContext&)
				{
					SceneData data;
					ActorRecord actor;
					actor.Name = "Fixture";
					ComponentRecord c;
					c.Name = "Lifetime";
					c.Type = scenario == "invalid-initial" ? "MissingType" : "Lifetime";
					actor.Components.push_back(c);
					data.Actors.push_back(actor);
					return data;
				});
			}
			catch (const std::runtime_error&)
			{
				caught = true;
			}
			const bool constructed = scenario != "invalid-initial" && scenario != "initialize-failure";
			Check(caught && game.Shutdowns == 1, "Expected failure did not unwind through Shutdown");
			Check(game.Counts.Begins == (constructed ? 1 : 0) && game.Counts.Ends == game.Counts.Begins &&
			          game.Counts.Destroyed == (constructed ? 1 : 0),
			      "Failure leaked or repeated component lifecycle");
			Check(game.Failures == (scenario == "invalid-initial" ? 1 : 0), "Wrong scene failure callback count");
		}
		const auto diagnostics = GraphicsEngine::Get().CollectDeviceDiagnostics();
		for (const auto& error : diagnostics.Errors)
		{
			std::cerr << error << '\n';
		}
		Check(diagnostics.Errors.empty(), "D3D debug errors");
		std::cout << (diagnostics.Available ? "PASS: clean D3D debug queue\n" : "D3D debug queue unavailable\n");
		std::cout << "PASS: " << scenario << "\n";
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
