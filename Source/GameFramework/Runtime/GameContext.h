#pragma once
#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include "GameFramework/Scenes/SceneBuilder.h"
#include "GameFramework/Input/GameInput.h"
#include "GameFramework/World/World.h"
#include "Vector2.hpp"

class GameApplication;

// Available during game callbacks; do not retain input references between callbacks.
// The game-facing facade for the active session, owned by GameApplication.
// Components may retain a context reference for their session lifetime, but should
// access it only from serialized gameplay callbacks. It is not a global service
// locator or a thread-safe interface for arbitrary background work.
class GameContext
{
public:
    ~GameContext() { myWorld->Shutdown(); }
	// World owns actors, and actors own components. Returned pointers/references are
	// borrowed; creating an actor does not transfer ownership to the game.
	World& GetWorld() { return *myWorld; }
	// Input is replaced when the host enters a new phase/frame. Read it during the
	// callback; do not retain a reference as a historical input sample. FixedUpdate
	// and Update have separate edge consumption, while LateUpdate shares Update input.
	const GameInput& GetInput() const { return myInput; }
	const std::filesystem::path& GetContentRoot() const { return myContentRoot; }
	// Initial graphics client size. Resize propagation is not implemented yet.
	CommonUtilities::Vector2u GetClientSize() const { return myClientSize; }
    void SetActiveCamera(CameraComponent* camera)
    {
        if (camera && &camera->GetWorld() != myWorld.get()) throw std::invalid_argument("Camera belongs to another world");
        myCamera = camera ? camera->GetHandle<CameraComponent>() : ComponentHandle<CameraComponent>{};
    }
    // The callback executes on the platform thread with gameplay stopped, so the
    // existing synchronous asset path is safe. Capture owned data or session objects.
    using SceneFactory = std::function<SceneBuildResult(const GameInput*)>;
    void RequestScene(SceneFactory factory)
    {
        std::scoped_lock lock(mySceneMutex);
        mySceneFactory = std::move(factory); mySceneRequested = true;
    }
	// Request an orderly exit at the host-loop boundary; this does not interrupt the
	// current callback or destroy the world immediately.
	void RequestQuit() { myQuitRequested = true; }

private:
	friend class GameApplication;
	GameInput myInput;
    std::unique_ptr<World> myWorld = std::make_unique<World>(&myInput);
	std::filesystem::path myContentRoot;
	CommonUtilities::Vector2u myClientSize;
	ComponentHandle<CameraComponent> myCamera;
    std::mutex mySceneMutex;
    SceneFactory mySceneFactory;
    std::atomic<bool> mySceneRequested = false;
	std::atomic<bool> myQuitRequested = false;
};
