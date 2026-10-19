#include "RegisterCustomComponents.h"
#include "CustomComponentRegistry.h"
#include "SpinComponent.h"
#include "Components/ScriptedEvents/ScriptedEventComponent.h"
#include "Components/ScriptedEvents/CollisionScriptTriggerComponent.h"
#include "Components/ScriptedEvents/ScriptedActionComponent.h"
#include <GameFramework/Components/AnimatorComponent.h>
#include <Application/Game/BehaviourTree/BehaviourTreeComponent.h>

void RegisterCustomComponents(CustomComponentRegistry& aRegistry)
{
	aRegistry.Register<SpinComponent>("SpinComponent");
	aRegistry.Register<AnimatorComponent>("AnimatorComponent");
	aRegistry.Register<BehaviourTreeComponent>("BehaviourTreeComponent");

	// --- Scripted Events ---
	aRegistry.Register<ScriptedEventComponent>("ScriptedEventComponent");
	aRegistry.Register<CollisionScriptTriggerComponent>("CollisionScriptTriggerComponent");
	aRegistry.Register<WaitActionComponent>("WaitActionComponent");
	aRegistry.Register<MoveActionComponent>("MoveActionComponent");
	aRegistry.Register<PlayAnimationActionComponent>("PlayAnimationActionComponent");
	aRegistry.Register<SetAnimationVariableActionComponent>("SetAnimationVariableActionComponent");
	aRegistry.Register<PlayAudioActionComponent>("PlayAudioActionComponent");
	aRegistry.Register<LockControlActionComponent>("LockControlActionComponent");
	aRegistry.Register<UnlockControlActionComponent>("UnlockControlActionComponent");
	aRegistry.Register<SetActorActiveActionComponent>("SetActorActiveActionComponent");
	aRegistry.Register<SetComponentEnabledActionComponent>("SetComponentEnabledActionComponent");
}
