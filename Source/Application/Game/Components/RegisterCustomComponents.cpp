#include "RegisterCustomComponents.h"
#include "SpinComponent.h"
#include "Components/ScriptedEvents/ScriptedEventComponent.h"
#include "Components/ScriptedEvents/ScriptedActionComponent.h"
#include <GameFramework/Components/AnimatorComponent.h>
#include <GameFramework/Components/CustomComponentRegistry.h>
#include <Application/Game/BehaviourTree/BehaviourTreeComponent.h>
#include <Application/Game/Controllers/ControllerComponent.h>
#include <Player/Components/PlayerController.h>
#include <Player/Components/PlayerMovementComponent.h>
#include <Player/Components/AbilityComponent.h>

void RegisterCustomComponents(CustomComponentRegistry& aRegistry)
{
	aRegistry.Register<SpinComponent>("SpinComponent");
	aRegistry.Register<AnimatorComponent>("AnimatorComponent");

	// --- Enemy ---
	aRegistry.Register<BehaviourTreeComponent>("BehaviourTreeComponent");
	aRegistry.Register<ControllerComponent>("ControllerComponent");

	// --- Scripted Events ---
	aRegistry.Register<ScriptedEventComponent>("ScriptedEventComponent");
	aRegistry.Register<WaitActionComponent>("WaitActionComponent");
	aRegistry.Register<MoveActionComponent>("MoveActionComponent");
	aRegistry.Register<PlayAnimationActionComponent>("PlayAnimationActionComponent");
	aRegistry.Register<SetAnimationVariableActionComponent>("SetAnimationVariableActionComponent");
	aRegistry.Register<PlayAudioActionComponent>("PlayAudioActionComponent");
	aRegistry.Register<LockControlActionComponent>("LockControlActionComponent");
	aRegistry.Register<UnlockControlActionComponent>("UnlockControlActionComponent");
	aRegistry.Register<SetActorActiveActionComponent>("SetActorActiveActionComponent");
	aRegistry.Register<SetComponentEnabledActionComponent>("SetComponentEnabledActionComponent");

	// --- Player ---
	aRegistry.Register<PlayerController>("PlayerControllerComponent");
	aRegistry.Register<PlayerMovementComponent>("PlayerMovementComponent");
	aRegistry.Register<AbilityComponent>("AbilityComponent");
}
