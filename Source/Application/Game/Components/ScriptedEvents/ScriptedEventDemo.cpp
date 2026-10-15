#include "ScriptedEventDemo.h"
#include "ScriptedActionComponent.h"
#include "ScriptedSequenceComponent.h"
#include "GameFramework/World/World.h"
#include "EnemyComponent.h"
#include "GameFramework/AudioManager.h"

void ConfigureScriptedEventDemo(World& world)
{
	Actor* player = world.FindActor("Player");
	Actor* enemy = world.FindActor("Enemy");
	if (!player || !enemy)
	{
		return;
	}
	Actor* start = world.SpawnActor("ScriptedDemoStart");
	start->GetTransform().SetWorldPosition(player->GetTransform().GetWorldPosition());
	Actor* destination = world.SpawnActor("ScriptedDemoDestination");
	destination->GetTransform().SetWorldPosition(player->GetTransform().GetWorldPosition() + CommonUtilities::Vector3f{200, 0, 0});
	Actor* sequenceActor = world.SpawnActor("ScriptedEventDemo");
	auto* sequence = sequenceActor->AddComponent<ScriptedSequenceComponent>("Sequence");
	sequence->Replayable = true;
	sequence->TriggerAction = "PlayScriptedEventDemo";
	auto* lock = sequenceActor->AddComponent<LockControlActionComponent>("LockPlayer");
	lock->TargetActor = player->GetName();
	if (auto* ai = enemy->GetComponent<EnemyComponent>())
	{
		auto* disable = sequenceActor->AddComponent<SetComponentEnabledActionComponent>("PauseEnemy");
		disable->TargetActor = enemy->GetName();
		disable->TargetComponent = ai->GetName();
		disable->Enabled = false;
		auto* enable = sequenceActor->AddComponent<SetComponentEnabledActionComponent>("ReleaseEnemy");
		enable->Step = 3;
		enable->TargetActor = enemy->GetName();
		enable->TargetComponent = ai->GetName();
	}
	auto* move = sequenceActor->AddComponent<MoveActionComponent>("WalkOut");
	move->Step = 1;
	move->TargetActor = player->GetName();
	move->DestinationActor = destination->GetName();
	move->Duration = 2.0f;
	auto* animation = sequenceActor->AddComponent<PlayAnimationActionComponent>("EnemyAnimation");
	animation->Step = 1;
	animation->TargetActor = enemy->GetName();
	animation->Animation = "Idle";
	animation->WaitForCompletion = false;
	auto* audio = sequenceActor->AddComponent<PlayAudioActionComponent>("EnemySound");
	audio->Step = 1;
	audio->TargetActor = enemy->GetName();
	audio->Sound = eHeavyRoar;
	audio->Positional = true;
	audio->WaitForCompletion = false;
	auto* returnMove = sequenceActor->AddComponent<MoveActionComponent>("WalkBack");
	returnMove->Step = 2;
	returnMove->TargetActor = player->GetName();
	returnMove->DestinationActor = start->GetName();
	returnMove->Duration = 2.0f;
	auto* unlock = sequenceActor->AddComponent<UnlockControlActionComponent>("UnlockPlayer");
	unlock->Step = 3;
	unlock->TargetActor = player->GetName();
}
