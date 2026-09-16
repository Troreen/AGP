#pragma once
class GameContext;

// All callbacks are serialized. Mutate gameplay state only inside these callbacks.
// Start here when creating a game. Implement one IGame in the application project;
// GameFramework owns the loop and calls these hooks. Actor-specific behavior belongs
// in Components, while these hooks coordinate the whole game session.
//
// Callbacks never overlap. Initialize/Shutdown run on the calling thread; frame hooks
// may run on the gameplay worker. Do not depend on a permanent OS thread identity.
// All delta times are seconds.
class IGame
{
public:
	virtual ~IGame() = default;
	// Build the initial scene and load shared assets before rendering starts.
	// The context and its World remain available until Shutdown completes.
	virtual void Initialize(GameContext&) = 0;
	// Zero or more calls before each Update, using a constant simulation step.
	// The engine ticks actor/component FixedUpdate immediately after this hook.
	// Use for simulation rules; it is not tied to the number of displayed frames.
	virtual void FixedUpdate(GameContext&, float) {}
	// One call per gameplay frame, followed by all component Update and LateUpdate
	// calls. Use for session input and frame-time behavior. Do not tick World yourself.
	virtual void Update(GameContext&, float) {}
	// Runs after every component has completed Update and LateUpdate, before snapshot
	// creation. Use for final session-wide adjustments that need the completed world.
	virtual void LateUpdate(GameContext&, float) {}
	// Runs after gameplay work is joined, including when Initialize partially fails.
	// Release game-owned session resources here; tolerate incomplete initialization.
	// This is session teardown, not an implemented scene-transition mechanism.
	virtual void Shutdown(GameContext&) {}
};
