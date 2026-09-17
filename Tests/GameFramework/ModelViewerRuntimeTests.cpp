#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>
#include <GameFramework/GameApplication.h>
#include <GameFramework/GameContext.h>
#include <GameFramework/World.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Components/StaticMeshComponent.h>
#include <GameFramework/Components/SkeletalMeshComponent.h>
#include <GameFramework/Components/LightComponent.h>
#include <GameFramework/SceneService.h>
#include "ModelViewer.h"
#include "ModelViewerScene.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace
{
	void Check(bool value, const char* message)
	{
		if (!value)
		{
			throw std::runtime_error(message);
		}
	}

	class SampleGame final : public IGame
	{
	public:
		ModelViewer Sample;
		int Loads = 0, Frames = 0, Shutdowns = 0;
		bool ReloadRequested = false, Rendered = false;
		ActorRef OldCamera;
		std::chrono::steady_clock::time_point Deadline;

		void RegisterComponents(ComponentRegistry& registry) override
		{
			Sample.RegisterComponents(registry);
		}

		void Initialize(GameContext& context) override
		{
			Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(180);
			Sample.Initialize(context);
		}

		void OnSceneLoaded(GameContext& context, const SceneId& scene) override
		{
			if (scene.Value == "Empty")
			{
				Check(Loads == 2 && !OldCamera.Get() && !context.GetWorld().GetActiveCamera(), "Empty replacement retained old camera");
				++Loads;
				Frames = 0;
				return;
			}
			Check(scene.Value == "ModelViewer", "Actual sample loaded wrong scene");
			auto& world = context.GetWorld();
			auto* camera = world.FindActor("Camera Actor");
			Check(camera && world.GetActiveCamera() == camera->GetComponent<CameraComponent>(), "Actual sample camera missing");
			auto requireActor = [&world](const char* name)
			{
				auto* actor = world.FindActor(name);
				Check(actor != nullptr, "Actual authored actor missing");
				return actor;
			};
			Check(requireActor("SM_Chest Actor")->GetComponent<StaticMeshComponent>(), "Actual chest component missing");
			Check(requireActor("TGA Bro Actor")->GetComponent<SkeletalMeshComponent>(), "Actual skeletal component missing");
			Check(requireActor("Directional Light Actor")->GetComponent<DirectionalLightComponent>(), "Actual directional light missing");
			Check(requireActor("Warm Character Point Actor")->GetComponent<PointLightComponent>(), "Actual point light missing");
			Check(requireActor("Spot Light Actor")->GetComponent<SpotLightComponent>(), "Actual spotlight missing");
			if (Loads == 0)
			{
				OldCamera = camera->GetRef();
			}
			else
			{
				Check(!OldCamera.Get(), "Real-content reload retained old identity");
				OldCamera = camera->GetRef();
			}
			++Loads;
			Frames = 0;
		}

		void OnSceneLoadFailed(GameContext&, const SceneLoadError& error) override
		{
			for (const auto& diagnostic : error.Diagnostics)
			{
				std::cerr << diagnostic.File << ':' << diagnostic.Actor << ':' << diagnostic.Component << ':' << diagnostic.Property << ' '
				          << diagnostic.Code << ' ' << diagnostic.Message << '\n';
			}
			throw std::runtime_error("Actual ModelViewer scene source failed validation");
		}

		void Update(GameContext& context, float delta) override
		{
			Check(std::chrono::steady_clock::now() < Deadline, "Real-content host timed out");
			Sample.Update(context, delta);
			++Frames;
			const auto stats = GraphicsEngine::Get().GetLastRenderStats();
			const bool rendered =
			    stats.TotalRenderItems >= 5 && stats.VisibleRenderItems > 0 && stats.BlendedRenderItems > 0 && stats.TotalLights == 3;
			if (Loads == 1 && Frames >= 10 && rendered && !ReloadRequested)
			{
				ReloadRequested = true;
				Check(context.GetScenes().Reload(), "Actual sample reload refused");
			}
			if (Loads == 2 && Frames >= 20 && rendered && !Rendered)
			{
				Rendered = true;
				Check(context.GetScenes().Load(SceneId{"Empty"}), "Empty replacement request refused");
			}
			if (Loads == 3 && Frames >= 10 && stats.TotalRenderItems == 0 && stats.TotalLights == 0)
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

	class SampleSource final : public GameFrameworkIntegration::ISceneSource
	{
		ModelViewerScene Sample;

		GameFrameworkIntegration::SceneSourceResult Load(const SceneId& id, GameFrameworkIntegration::SceneLoadContext& context) override
		{
			if (id.Value == "Empty")
			{
				return {SceneData{}, {}};
			}
			return Sample.Load(id, context);
		}
	};
}

int main(int argc, char** argv)
{
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	try
	{
		SampleGame game;
		GameApplication::Config config;
		config.ShowWindow = false;
		config.Width = 640;
		config.Height = 360;
		config.ThreadedUpdate = argc < 2 || std::string(argv[1]) != "sync";
		config.ContentRoot = std::filesystem::current_path() / "Assets";
		GameFrameworkIntegration::ApplicationSetup setup;
		setup.SceneSource = std::make_unique<SampleSource>();
		const int result = GameApplication{}.Run(game, config, std::move(setup));
		Check(result == 0 && game.Loads == 3 && game.Rendered && game.Shutdowns == 1, "Actual sample lifecycle/render exercise incomplete");
		const auto diagnostics = GraphicsEngine::Get().CollectDeviceDiagnostics();
		for (const auto& error : diagnostics.Errors)
		{
			std::cerr << error << '\n';
		}
		Check(diagnostics.Errors.empty(), "D3D debug layer reported errors/corruption");
		std::cout << (diagnostics.Available ? "PASS: D3D debug queue contains no ERROR/CORRUPTION messages\n"
		                                    : "UNAVAILABLE: D3D debug queue; no clean-debug-layer claim\n");
		std::cout << "PASS: actual ModelViewer registration, assets, authored source, callbacks, reload and hidden rendering ("
		          << (config.ThreadedUpdate ? "threaded" : "sync") << ")\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
