#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>
#include "EnumKeyCode.h"
#include "GameFramework/Runtime/GameApplication.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/LightComponent.h"
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

	void ConfigureWorld(World& world) override
	{
		Sample.ConfigureWorld(world);
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
			Check(Loads == 3 && !world.FindActor("Camera Actor") && world.FindActor("__DebugCamera") && world.GetActiveCamera(),
			      "Empty scene did not install the debug camera");
			return;
		}
		Check(name == "ChestMaterials", "Wrong scene");
		Check(&ServiceLocator::GetInstance().GetInputSystem() == &context.GetInputSystem(), "ServiceLocator input service mismatch");
		Check(&ServiceLocator::GetInstance().GetAssetRegistry() == &AssetRegistry::Get(), "ServiceLocator asset service mismatch");
		Check(bool(AssetRegistry::Get().ResolveMaterial(AssetId{"Shaders/CubeMaterial.mat"})), "Flat material did not load");
		const auto parameterInstance = AssetRegistry::Get().ResolveMaterial(AssetId{"ChestMaterial_Alpha1"});
		Check(bool(parameterInstance), AssetRegistry::Get().GetLastError().c_str());
		Check(bool(AssetRegistry::Get().ResolveMaterial(AssetId{"Shaders/ChestMaterial_Alpha2.mat"})), "Material texture overrides did not load");
		Check(world.FindActor("__DebugCamera") && world.GetActiveCamera(), "Imported scene did not install the debug camera");
		auto* chest = world.FindActor("Chest_Opaque");
		Check(chest && chest->GetComponent<StaticMeshComponent>(), "Imported chest mesh missing");
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
		Check(context.GetSceneName() == "ChestMaterials" && context.GetWorld().FindActor("Old-scene-only") && context.GetWorld().GetActiveCamera(),
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
		    stats.TotalRenderItems >= 3 && stats.VisibleRenderItems > 0 && stats.TotalLights >= 3;
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

class ChestShowcaseGame final : public IGame
{
public:
	int Frames = 0;
	Game Sample;
	PointLightComponent* OrbitLight = nullptr;
	CommonUtilities::Vector3f InitialOrbitPosition{};

	void ConfigureWorld(World& world) override
	{
		Sample.ConfigureWorld(world);
	}

	void Initialize(GameContext& context) override
	{
		Check(context.LoadScene("ChestMaterials"), "Chest material scene request was rejected");
	}

	void OnSceneLoaded(GameContext& context, const std::string& name) override
	{
		Check(name == "ChestMaterials", "Wrong chest material scene loaded");
		for (const char* actorName : {"Chest_Opaque", "Chest_AmberGlass", "Chest_MarbleGlass"})
		{
			Actor* actor = context.GetWorld().FindActor(actorName);
			Check(actor && actor->GetComponent<StaticMeshComponent>(), "Chest showcase mesh was not constructed");
		}
		Actor* sun = context.GetWorld().FindActor("SunLight");
		Actor* doubleLight = context.GetWorld().FindActor("DoubleLight");
		auto* center = doubleLight ? dynamic_cast<PointLightComponent*>(doubleLight->FindComponent("CenterPointLight")) : nullptr;
		OrbitLight = doubleLight ? dynamic_cast<PointLightComponent*>(doubleLight->FindComponent("OrbitPointLight")) : nullptr;
		Check(sun && sun->GetComponent<DirectionalLightComponent>(), "Directional light was not constructed");
		Check(center && OrbitLight && doubleLight->FindComponent("Spin"), "DoubleLight code configuration is incomplete");
		Check(center->GetTransform().GetLocalPosition().LengthSqr() == 0 &&
		      OrbitLight->GetTransform().GetLocalPosition().LengthSqr() > 0,
		      "DoubleLight local offsets are incorrect");
		InitialOrbitPosition = OrbitLight->GetWorldPosition();
	}

	void Update(GameContext& context, float) override
	{
		const auto stats = GraphicsEngine::Get().GetLastRenderStats();
		const bool orbitMoved = OrbitLight && (OrbitLight->GetWorldPosition() - InitialOrbitPosition).LengthSqr() > 0.000001f;
		if (++Frames >= 5 && orbitMoved && stats.TotalRenderItems >= 3 && stats.VisibleRenderItems > 0 && stats.TotalLights >= 3)
		{
			context.RequestQuit();
		}
		Check(Frames < 600, "Chest material scene did not render");
	}
};

class OverlayOnlyGame final : public IGame
{
public:
	int Frames = 0;

	void Initialize(GameContext& context) override
	{
		InputDeviceFrame frame;
		frame.KeysDown[static_cast<size_t>(EKeyCode::F6)] = true;
		context.GetInputSystem().Update(frame);
	}

	void Update(GameContext& context, float) override
	{
		const auto stats = GraphicsEngine::Get().GetLastRenderStats();
		if (stats.TextDrawCalls > 0 && stats.RenderedGlyphs > 0)
		{
			context.RequestQuit();
		}
		Check(++Frames < 120, "Overlay did not render without an active camera");
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

	void ConfigureWorld(World& world) override
	{
		auto* actor = world.FindActor("Fixture");
		if (!actor) return;
		auto* component = actor->AddComponent<Lifetime>("Lifetime");
		component->Counts = &Counts;
		component->Failure = Scenario;
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

int RunRenderPassControlsTest()
{
	GraphicsEngine& graphics = GraphicsEngine::Get();
	Check(std::string(graphics.GetRenderPassName()) == "Lit", "Render-pass test did not start on Lit");

	InputSystem input;
	InstallDefaultInputBindings(input);
	InputSubscription previous = input.Subscribe(InputActions::PreviousRenderPass, [&graphics](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started) graphics.SelectPreviousRenderPass();
	});
	InputSubscription next = input.Subscribe(InputActions::NextRenderPass, [&graphics](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started) graphics.SelectNextRenderPass();
	});

	InputDeviceFrame frame;
	frame.KeysDown[static_cast<size_t>(EKeyCode::F5)] = true;
	input.Update(frame);
	Check(std::string(graphics.GetRenderPassName()) == "Shadows (Directional)", "F5 did not wrap to the previous render pass");

	input.Update({});
	frame = {};
	frame.KeysDown[static_cast<size_t>(EKeyCode::F6)] = true;
	input.Update(frame);
	Check(std::string(graphics.GetRenderPassName()) == "Lit", "F6 did not advance to the next render pass");

	input.Update({});
	frame = {};
	frame.KeysDown[static_cast<size_t>(EKeyCode::F6)] = true;
	input.Update(frame);
	Check(std::string(graphics.GetRenderPassName()) == "Albedo (sRGB)", "F6 did not update to the next render-pass name");
	input.Update({});
	frame = {};
	frame.KeysDown[static_cast<size_t>(EKeyCode::F5)] = true;
	input.Update(frame);
	Check(std::string(graphics.GetRenderPassName()) == "Lit", "F5 did not update to the previous render-pass name");

	std::cout << "PASS: F5 previous and F6 next render-pass controls\n";
	return 0;
}

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
	if (scenario == "render-pass-controls")
	{
		return RunRenderPassControlsTest();
	}
	GameApplication::Config config;
	config.ShowWindow = false;
	config.Width = 640;
	config.Height = 360;
	config.ContentRoot = std::filesystem::current_path() / "Content";
	config.EnableRenderDiagnostics = scenario == "text-overlay";
	try
	{
		if (scenario == "text-overlay")
		{
			OverlayOnlyGame game;
			GameApplication{}.Run(game, config);
			const auto stats = GraphicsEngine::Get().GetLastRenderStats();
			Check(stats.TextDrawCalls > 0 && stats.RenderedGlyphs > 0, "Overlay statistics stayed at zero");
		}
		else if (scenario == "chest-materials")
		{
			ChestShowcaseGame game;
			GameScene source;
			GameApplication{}.Run(game, config, [&](const std::string& name, SceneLoadContext& context)
			{
				return source.Load(name, context);
			});
		}
		else if (scenario == "sample")
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
					throw std::runtime_error("Broken/Component: MissingType");
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
					if (scenario == "invalid-initial") throw std::runtime_error("Fixture/Lifetime: MissingType");
					SceneData data;
					ActorRecord actor;
					actor.Name = "Fixture";
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
