#pragma once
#include <atomic>
#include <filesystem>
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
	// World owns actors, and actors own components. Returned pointers/references are
	// borrowed; creating an actor does not transfer ownership to the game.
	World& GetWorld() { return myWorld; }
	// Input is replaced when the host enters a new phase/frame. Read it during the
	// callback; do not retain a reference as a historical input sample. FixedUpdate
	// and Update have separate edge consumption, while LateUpdate shares Update input.
	const GameInput& GetInput() const { return myInput; }
	const std::filesystem::path& GetContentRoot() const { return myContentRoot; }
	// Initial graphics client size. Resize propagation is not implemented yet.
	CommonUtilities::Vector2u GetClientSize() const { return myClientSize; }
	// Select a live actor with a CameraComponent from this world. This raw pointer
	// does not keep the actor alive. nullptr stops new snapshot construction; it does
	// not clear a previously presented image. Scene unloading needs a future lifetime API.
	void SetActiveCamera(Actor* camera) { myCamera = camera; }
	// Request an orderly exit at the host-loop boundary; this does not interrupt the
	// current callback or destroy the world immediately.
	void RequestQuit() { myQuitRequested = true; }

private:
	friend class GameApplication;
	World myWorld;
	GameInput myInput;
	std::filesystem::path myContentRoot;
	CommonUtilities::Vector2u myClientSize;
	Actor* myCamera = nullptr;
	std::atomic<bool> myQuitRequested = false;
};
