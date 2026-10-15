#include "Components/ScriptedEvents/ScriptedActionComponent.h"
#include "Components/ScriptedEvents/ScriptedSequenceComponent.h"
#include "GameFramework/World/World.h"
#include "Components/RegisterCustomComponents.h"
#include "Components/CustomComponentRegistry.h"
#include "GameFramework/Scenes/SceneWorldBuilder.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "Player/PlayerController.h"
#include "Player/PlayerMovementComponent.h"
#include "GameFramework/Components/AnimatorComponent.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/Animation/AnimationManager.h"
#include "GameFramework/ServiceLocator.h"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
	void Check(bool value, const char* message)
	{
		if (!value) { throw std::runtime_error(message); }
	}

	void Advance(World& world, float duration)
	{
		while (duration > 0)
		{
			const float delta = std::min(duration, 0.25f);
			world.Update(delta);
			duration -= delta;
		}
	}

	void TestExportedCustomFields()
	{
		CustomComponentRegistry registry;
		RegisterCustomComponents(registry);
		SceneData scene;
		ActorRecord event;
		event.Name = "ExportedEvent";
		CustomComponentData coordinator;
		coordinator.Common.Name = "Sequence";
		coordinator.CustomType = "ScriptedSequenceComponent_C";
		coordinator.Variables.BoolVariables.emplace("Replayable", true);
		event.Components.push_back(coordinator);
		CustomComponentData move;
		move.Common.Name = "MoveFromExport";
		move.CustomType = "MoveActionComponent_C";
		move.Variables.IntVariables.emplace("Step", 3);
		move.Variables.BoolVariables.emplace("WaitForCompletion", true);
		move.Variables.StringVariables.emplace("TargetActor", "Target");
		move.Variables.StringVariables.emplace("DestinationActor", "Marker");
		move.Variables.FloatVariables.emplace("Duration", 0.5f);
		event.Components.push_back(move);
		scene.Actors.push_back(event);
		ActorRecord target;
		target.Name = "Target";
		scene.Actors.push_back(target);
		ActorRecord marker;
		marker.Name = "Marker";
		marker.Transform.Position = {10, 0, 0};
		scene.Actors.push_back(marker);
		AssetRegistry assets;
		SceneWorldBuilder builder;
		auto world = builder.BuildWorld(scene, assets, {}, {1280, 720}, {},
			[&registry](Actor& actor, const CustomComponentData& data) { return registry.Create(actor, data); });
		auto* actor = world->FindActor("ExportedEvent");
		auto* sequence = actor->GetComponent<ScriptedSequenceComponent>();
		auto* action = actor->GetComponent<MoveActionComponent>();
		Check(sequence && sequence->Replayable && action && action->Step == 3 && action->WaitForCompletion,
			"Registry/builder did not apply exported sequence and action variables");
		Check(action->TargetActor == "Target" && action->DestinationActor == "Marker" && action->Duration == 0.5f,
			"Exported action references or duration changed");
		world->BeginPlay();
		Check(sequence->Play(), "Sequence built through custom export records did not start");
		Advance(*world, 0.5f);
		Check(sequence->HasCompleted() && world->FindActor("Target")->GetTransform().GetWorldPosition().x == 10,
			"Exported custom fields did not drive movement");
	}

	ScriptedSequenceComponent* Sequence(World& world, const char* name)
	{
		return world.SpawnActor(name)->AddComponent<ScriptedSequenceComponent>("Sequence");
	}

	WaitActionComponent* Wait(ScriptedSequenceComponent* sequence, const char* name, int step, float duration)
	{
		auto* action = sequence->GetOwner()->AddComponent<WaitActionComponent>(name);
		action->Step = step;
		action->Duration = duration;
		return action;
	}

	MoveActionComponent* Move(ScriptedSequenceComponent* sequence, int step, float duration)
	{
		auto* action = sequence->GetOwner()->AddComponent<MoveActionComponent>("Move");
		action->TargetActor = "Target";
		action->DestinationActor = "Marker";
		action->Step = step;
		action->Duration = duration;
		return action;
	}

	void CheckPosition(const Actor* actor, float expected, const char* message)
	{
		Check(std::abs(actor->GetTransform().GetWorldPosition().x - expected) < 0.001f, message);
	}

	void TestOrderingAndOverlap()
	{
		World world;
		auto* target = world.SpawnActor("Target");
		world.SpawnActor("Marker")->GetTransform().SetWorldPosition({8, 0, 0});
		auto* sequence = Sequence(world, "Ordering");
		auto* first = Wait(sequence, "First", 0, 0.5f);
		auto* sameStep = Wait(sequence, "SameStep", 0, 1.0f);
		auto* move = Move(sequence, 1, 1.0f);
		world.BeginPlay();
		Check(sequence->Play(), "Ordered sequence did not start");
		Check(first->IsStarted() && sameStep->IsStarted() && !move->IsStarted(), "Same-step actions must start together");
		Advance(world, 0.5f);
		Check(first->IsFinished() && !sameStep->IsFinished() && !move->IsStarted(), "Step must wait for its slowest waiting action");
		Advance(world, 0.5f);
		Check(move->IsStarted(), "Next step did not start after waiting actions finished");
		Advance(world, 0.5f);
		CheckPosition(target, 4, "Movement did not interpolate halfway");
		Advance(world, 0.5f);
		CheckPosition(target, 8, "Movement did not reach marker at its exact duration");
		Check(sequence->HasCompleted() && !sequence->IsRunning(), "Sequence did not complete");

		auto* overlap = Sequence(world, "Overlap");
		auto* background = Wait(overlap, "Background", 0, 2.0f);
		background->WaitForCompletion = false;
		auto* next = Wait(overlap, "Next", 1, 1.0f);
		Check(overlap->Play(), "Overlap sequence did not start");
		world.Update(0.25f);
		Check(next->IsStarted() && !background->IsFinished(), "Non-waiting action blocked the next step");
		Advance(world, 0.5f);
		Check(!next->IsFinished() && !background->IsFinished(), "Overlapping actions completed prematurely");
		overlap->Cancel();
	}

	void TestPersistentChangesAndReplay()
	{
		World world;
		auto* target = world.SpawnActor("Target");
		target->SetActive(false);
		auto* behavior = target->AddComponent<Component>("Behavior");
		behavior->SetEnabled(false);
		auto* sequence = Sequence(world, "Reveal");
		auto* activate = sequence->GetOwner()->AddComponent<SetActorActiveActionComponent>("RevealActor");
		activate->TargetActor = "Target";
		auto* enable = sequence->GetOwner()->AddComponent<SetComponentEnabledActionComponent>("ReleaseAI");
		enable->TargetActor = "Target";
		enable->TargetComponent = "Behavior";
		world.BeginPlay();
		Check(sequence->Play(), "Reveal sequence did not start");
		world.Update(0.1f);
		Check(sequence->HasCompleted() && target->IsActive() && behavior->IsEnabled(), "Authored activation or AI handoff did not persist");
		Check(!sequence->Play(), "Once-only sequence replayed");
		sequence->Replayable = true;
		Check(sequence->Play(), "Replayable sequence refused another run");
		world.Update(0.1f);
		Check(sequence->HasCompleted(), "Replayed sequence did not complete");
	}

	void TestClaimsAndValidation()
	{
		World world;
		auto* target = world.SpawnActor("Target");
		world.SpawnActor("Marker")->GetTransform().SetWorldPosition({10, 0, 0});
		auto* first = Sequence(world, "First");
		Move(first, 0, 2.0f);
		auto* second = Sequence(world, "Second");
		Move(second, 0, 2.0f);
		world.BeginPlay();
		Check(first->Play(), "First claimant did not start");
		Advance(world, 0.5f);
		Check(!second->Play() && !second->GetLastError().empty(), "Conflicting claimant was accepted");
		CheckPosition(target, 2.5f, "Rejected sequence changed its target");
		first->Cancel();
		Check(!first->IsRunning() && !first->HasCompleted(), "Cancellation reported successful completion");
		Check(second->Play(), "Cancellation did not release actor claims");
		second->Cancel();

		auto* invalid = Sequence(world, "Invalid");
		auto* disable = invalid->GetOwner()->AddComponent<SetActorActiveActionComponent>("WouldHideTarget");
		disable->TargetActor = "Target";
		disable->Active = false;
		auto* missing = invalid->GetOwner()->AddComponent<SetActorActiveActionComponent>("MissingTarget");
		missing->Step = 1;
		missing->TargetActor = "Missing";
		Check(!invalid->Play() && !invalid->GetLastError().empty(), "Missing target passed preflight");
		Check(target->IsActive() && !disable->IsStarted(), "Preflight failure allowed an earlier action to mutate the scene");
	}

	void TestDestruction()
	{
		World world;
		auto* target = world.SpawnActor("Target");
		world.SpawnActor("Marker")->GetTransform().SetWorldPosition({10, 0, 0});
		auto* sequence = Sequence(world, "DestroyedTarget");
		Move(sequence, 0, 2.0f);
		world.BeginPlay();
		Check(sequence->Play(), "Destruction sequence did not start");
		target->Destroy();
		world.Update(0.1f);
		Check(!sequence->IsRunning() && !sequence->HasCompleted() && !sequence->GetLastError().empty(), "Destroyed target did not cancel safely");
		world.SpawnActor("Target");
		auto* successor = Sequence(world, "Successor");
		Move(successor, 0, 1.0f);
		Check(successor->Play(), "Destroyed-target cancellation retained claims");
		successor->Cancel();

		auto* removed = Sequence(world, "RemovedAction");
		auto* action = Wait(removed, "Removed", 0, 5.0f);
		Check(removed->Play(), "Removed-action sequence did not start");
		action->Destroy();
		world.Update(0.1f);
		Check(!removed->IsRunning() && !removed->GetLastError().empty(), "Removed action did not cancel safely");
		world.Clear();
	}

	void TestOverlappingMovementRejected()
	{
		World world;
		auto* target = world.SpawnActor("Target");
		world.SpawnActor("Marker")->GetTransform().SetWorldPosition({10, 0, 0});
		auto* sequence = Sequence(world, "ConflictingMoves");
		auto* first = Move(sequence, 0, 2.0f);
		first->WaitForCompletion = false;
		auto* second = sequence->GetOwner()->AddComponent<MoveActionComponent>("SecondMove");
		second->Step = 1;
		second->TargetActor = "Target";
		second->DestinationActor = "Marker";
		world.BeginPlay();
		Check(!sequence->Play() && !sequence->GetLastError().empty(), "Overlapping movement on one target was accepted");
		CheckPosition(target, 0, "Overlapping movement failure mutated the target");
	}

	void TestControlCleanup()
	{
		World world;
		auto* player = world.SpawnActor("Player");
		auto* movement = player->AddComponent<PlayerMovementComponent>("Movement");
		auto* controller = player->AddComponent<PlayerController>("Controller");
		// These public control operations do not need the camera/input services used by BeginPlay.
		movement->BeginPlay();
		movement->SetDestination({100, 0, 0});
		auto* sequence = Sequence(world, "LockPlayer");
		sequence->Replayable = true;
		auto* lock = sequence->GetOwner()->AddComponent<LockControlActionComponent>("Lock");
		lock->TargetActor = "Player";
		lock->TargetComponent = "Controller";
		Wait(sequence, "Hold", 0, 5.0f);
		Check(sequence->Play() && controller->IsScriptedControlLocked(), "Lock action did not take player control");
		movement->Update(0.1f);
		CheckPosition(player, 0, "Lock did not clear existing movement");
		Check(controller->IsEnabled() && movement->IsEnabled(), "Control lock disabled gameplay components");
		sequence->Cancel();
		Check(!controller->IsScriptedControlLocked(), "Cancellation did not restore player control");
		Check(sequence->Play() && controller->IsScriptedControlLocked(), "Control lock did not restart");
		auto* unlock = sequence->GetOwner()->AddComponent<UnlockControlActionComponent>("Unlock");
		unlock->TargetActor = "Player";
		Check(unlock->Start() && !controller->IsScriptedControlLocked(), "Implicit controller selection did not release an explicit controller lock");
		sequence->Cancel();
		movement->Update(0.1f);
		CheckPosition(player, 0, "Cancellation resumed an old destination");
		movement->SetScriptedMovement(true);
		movement->SetDestination({100, 0, 0});
		movement->Update(0.1f);
		CheckPosition(player, 0, "Normal movement interfered during scripted movement");
		movement->SetScriptedMovement(false);
		movement->SetDestination({100, 0, 0});
		movement->Update(0.1f);
		CheckPosition(player, 30, "Normal movement did not resume after scripted movement");
	}

	void TestSceneReplacementReleasesClaims()
	{
		World world;
		world.SpawnActor("Target");
		world.SpawnActor("Marker");
		auto* sequence = Sequence(world, "OldSceneEvent");
		Move(sequence, 0, 5.0f);
		world.BeginPlay();
		Check(sequence->Play(), "Old scene sequence did not start");
		world.Clear();
		world.SpawnActor("Target");
		world.SpawnActor("Marker");
		auto* replacement = Sequence(world, "NewSceneEvent");
		Move(replacement, 0, 1.0f);
		world.BeginPlay();
		Check(replacement->Play(), "Scene replacement retained stale actor claims");
		Advance(world, 1.0f);
		Check(replacement->HasCompleted(), "Replacement scene event did not complete");
	}

	std::shared_ptr<AnimationAsset> Clip(const char* name)
	{
		auto animation = std::make_shared<Animation>();
		animation->Name = name;
		animation->Frames.resize(2);
		animation->FramesPerSecond = 4;
		animation->Duration = 0.5f;
		return std::make_shared<AnimationAsset>(animation);
	}

	void TestAnimationPlaybackAndPreflight()
	{
		World world;
		auto* target = world.SpawnActor("Target");
		auto* animator = target->AddComponent<AnimatorComponent>("Animator");
		auto* mesh = target->AddComponent<SkeletalMeshComponent>("Mesh");
		mesh->SetMesh(std::make_shared<MeshAsset>(std::make_shared<Mesh>()));
		animator->SetMeshComponent(mesh);
		animator->AddAnimation("Idle", Clip("Idle"));
		animator->AddAnimation("Gesture", Clip("Gesture"));
		Check(animator->PlayAnimation("Idle", true), "Ordinary animation did not start");
		world.BeginPlay();
		world.Update(0.125f);
		auto* sequence = Sequence(world, "AnimationSequence");
		sequence->Replayable = true;
		auto* action = sequence->GetOwner()->AddComponent<PlayAnimationActionComponent>("GestureAction");
		action->TargetActor = "Target";
		action->Animation = "Gesture";
		Check(sequence->Play() && animator->IsScriptedAnimationPlaying(), "Scripted animation did not start");
		Check(animator->GetCurrentPlayBackState(true).CurrentFrame == 0, "Scripted clip did not start at frame zero");
		world.Update(0.25f);
		const auto cancelledPose = animator->GetCurrentPlayBackState(true);
		sequence->Cancel();
		world.Update(0.125f);
		const auto heldPose = animator->GetCurrentPlayBackState(true);
		Check(heldPose.AnimationName == "Gesture" && heldPose.CurrentFrame == cancelledPose.CurrentFrame &&
		          heldPose.Timer == cancelledPose.Timer && !heldPose.Active,
		      "Cancelling without a tree did not hold the scripted pose");
		Check(sequence->Play(), "Cancelled replayable animation did not restart");
		Check(animator->GetCurrentPlayBackState(true).CurrentFrame == 0 && animator->GetCurrentPlayBackState(true).Timer == 0,
		      "Restarted scripted clip retained old playback progress");
		Advance(world, 0.75f);
		Check(sequence->HasCompleted() && !animator->IsScriptedAnimationPlaying(), "Scripted clip completion did not finish the sequence");
		Check(animator->GetCurrentPlayBackState(true).AnimationName == "Gesture" && !animator->GetCurrentPlayBackState(true).Active,
		      "Completion without a tree did not hold the scripted pose");
		Check(animator->PlayAnimation("Idle", true), "Held pose prevented another animation from starting");

		mesh->SetMesh(nullptr);
		auto* invalid = Sequence(world, "MissingMesh");
		auto* disable = invalid->GetOwner()->AddComponent<SetActorActiveActionComponent>("WouldHideTarget");
		disable->TargetActor = "Target";
		disable->Active = false;
		auto* invalidClip = invalid->GetOwner()->AddComponent<PlayAnimationActionComponent>("InvalidClip");
		invalidClip->Step = 1;
		invalidClip->TargetActor = "Target";
		invalidClip->Animation = "Gesture";
		Check(!invalid->Play() && !invalid->GetLastError().empty(), "Missing mesh did not fail animation preflight");
		Check(target->IsActive() && !disable->IsStarted(), "Missing mesh preflight allowed an earlier mutation");
	}

	void TestAnimationTreeHandoff()
	{
		// Use the real tree loader with tiny CPU-only clips; no renderer or game content is needed.
		const auto fixtureRoot = std::filesystem::temp_directory_path() /
		                         ("AGP-scripted-handoff-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		const auto animationDirectory = fixtureRoot / "Animations" / "Animation Manager";
		std::filesystem::create_directories(animationDirectory / "Trees");
		std::ofstream(animationDirectory / "AnimationManager.json") << R"({
			"HandoffTree": {"Start state": "Idle", "Variables": [{"Name": "ShouldWalk", "Type": "Bool"}]}
		})";
		std::ofstream(animationDirectory / "AnimationDebug.json") << R"({"Debug Animation": {"Enabled": false}})";
		std::ofstream(animationDirectory / "Trees" / "HandoffTree.json") << R"({
			"Idle": {"Animation": "tree_idle", "IsLooping": true,
				"Transitions": [{"TransitionTo": "Walk", "Variable": "ShouldWalk", "Expected": true}]},
			"Walk": {"Animation": "tree_walk", "IsLooping": true}
		})";

		auto& services = ServiceLocator::GetInstance();
		services.Initialize();
		auto& assets = services.GetAssetRegistry();
		assets.Initialize(fixtureRoot);
		assets.Pin("tree_idle", Clip("Idle"));
		assets.Pin("tree_walk", Clip("Walk"));
		services.GetAnimationManager().Initialize(fixtureRoot);
		{
			World world;
			auto* target = world.SpawnActor("Target");
			auto* animator = target->AddComponent<AnimatorComponent>("Animator");
			auto* mesh = target->AddComponent<SkeletalMeshComponent>("Mesh");
			mesh->SetMesh(std::make_shared<MeshAsset>(std::make_shared<Mesh>()));
			animator->SetMeshComponent(mesh);
			animator->SetTree("HandoffTree");
			animator->AddAnimation("Gesture", Clip("Gesture"));
			world.BeginPlay();
			auto& tree = animator->GetTree();
			int entries = 0;
			int exits = 0;
			tree.GetCurrentAnimationState().OnEnterEvent().Subscribe([&entries]()
			{
				++entries;
			});
			tree.GetCurrentAnimationState().OnExitEvent().Subscribe([&exits]()
			{
				++exits;
			});

			Check(animator->BeginScriptedAnimation("Gesture", false), "Tree override did not start");
			tree.SetBool("ShouldWalk", true);
			world.Update(0.25f);
			Check(tree.GetCurrentAnimationState().GetName() == "Idle", "Tree transitioned during scripted playback");
			animator->EndScriptedAnimation();
			const auto resumed = animator->GetCurrentPlayBackState(true);
			Check(resumed.AnimationName == "Idle" && resumed.CurrentFrame == 0 && resumed.Timer == 0 && resumed.Active,
			      "Tree did not restart its current animation from frame zero");
			Check(entries == 0 && exits == 0, "Tree handoff repeated state entry or exit events");
			Check(tree.GetAnimationVariable("ShouldWalk").IsActive, "Tree handoff lost a variable change");
			world.Update(0.125f);
			Check(tree.GetCurrentAnimationState().GetName() == "Walk" && exits == 1,
			      "Tree did not resume normal transitions after scripted playback");

			// Even an override using the tree's own clip must restart when it gives control back.
			Check(animator->BeginScriptedAnimation("Walk", true), "Same-clip override did not start");
			world.Update(0.25f);
			animator->EndScriptedAnimation();
			const auto restarted = animator->GetCurrentPlayBackState(true);
			Check(restarted.AnimationName == "Walk" && restarted.CurrentFrame == 0 && restarted.Timer == 0 && restarted.Active,
			      "Same-clip handoff retained scripted playback progress");
		}
		services.KillServices();
		std::filesystem::remove_all(fixtureRoot);
	}
}

int main()
try
{
	TestExportedCustomFields();
	TestOrderingAndOverlap();
	TestPersistentChangesAndReplay();
	TestClaimsAndValidation();
	TestDestruction();
	TestOverlappingMovementRejected();
	TestControlCleanup();
	TestSceneReplacementReleasesClaims();
	TestAnimationPlaybackAndPreflight();
	TestAnimationTreeHandoff();
	std::cout << "PASS: scripted sequence ordering, movement, persistence, claims, replay and destruction\n";
	return 0;
}
catch (const std::exception& error)
{
	std::cerr << error.what() << '\n';
	return 1;
}
