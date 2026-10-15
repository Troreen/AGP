#include "ScriptedSequenceComponent.h"
#include "ScriptedActionComponent.h"
#include "GameFramework/Scenes/CustomComponentVariables.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"
#include "GameFramework/ServiceLocator.h"
#include "InputMapper.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>

namespace
{
	// Remember which sequence is using each actor, so another sequence cannot take control of it halfway through.
	// These pointers only identify actors; we never access actors through this list. EndPlay clears their reservations.
	std::unordered_map<const Actor*, ScriptedSequenceComponent*> actorClaims;
}

void ScriptedSequenceComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	Replayable = variables.GetOrDefault<bool>("Replayable", false);
	TriggerAction = variables.GetOrDefault<std::string>("TriggerAction", "");
}

void ScriptedSequenceComponent::BeginPlay()
{
	if (!TriggerAction.empty())
	{
		// TODO: Use a button to test events for now. A future trigger box can call Play() directly.
		myTriggerListener = ServiceLocator::GetInstance().GetInputMapper().AddEventListener(TriggerAction, [this](const CommonUtilities::InputEvent& event)
		{
			if (event.inputData.isPressed)
			{
				Play();
			}
		});
		myListeningForTrigger = true;
	}
}

bool ScriptedSequenceComponent::Play()
{
	// Ignore repeated triggers while running. By default, an event can only start once per scene load.
	if (myRunning || (myPlayed && !Replayable) || !IsEnabled() || !GetOwner()->IsActive())
	{
		return false;
	}
	myLastError.clear();

	// Gather the enabled actions on this event actor and put the lowest step numbers first.
	// Actions with the same step keep their attachment order and will start together.
	auto actions = GetOwner()->GetComponents<ScriptedActionComponent>();
	std::erase_if(actions, [](const auto* action)
	{
		return !action->IsEnabled();
	});
	std::stable_sort(actions.begin(), actions.end(), [](const auto* firstAction, const auto* secondAction)
	{
		return firstAction->Step < secondAction->Step;
	});

	if (GetOwner()->GetComponents<ScriptedSequenceComponent>().size() != 1)
	{
		myLastError = "A sequence Actor needs exactly one coordinator";
	}

	if (actions.empty())
	{
		myLastError = "Sequence has no enabled actions";
	}

	// Check the whole event before doing anything. A missing target in a later step must not leave earlier steps half-applied.
	for (auto* action : actions)
	{
		if (!myLastError.empty())
		{
			break;
		}

		if (action->Step < 0)
		{
			myLastError = "Step must be nonnegative";
			break;
		}

		if (!action->Validate(myLastError))
		{
			break;
		}

		if (action->ClaimsTarget())
		{
			// A claim reserves the actor for this event until it ends, including the gaps between its actions.
			Actor* targetActor = GetWorld().FindActor(action->TargetActor);
			if (targetActor == GetOwner())
			{
				myLastError = "Actions must target a separate Actor";
				break;
			}
			if (actorClaims.contains(targetActor) && actorClaims.at(targetActor) != this)
			{
				myLastError = "Actor already controlled by another sequence: " + action->TargetActor;
				break;
			}
		}
	}

	// Two clips or two moves cannot control the same actor at once. Different actors can act together,
	// and one actor can move while animating. To use the same control again, wait for the earlier action before a later step.
	for (std::size_t laterActionIndex = 0; laterActionIndex < actions.size() && myLastError.empty(); ++laterActionIndex)
	{
		for (std::size_t earlierActionIndex = 0; earlierActionIndex < laterActionIndex; ++earlierActionIndex)
		{
			const auto* earlierAction = actions[earlierActionIndex];
			const auto* laterAction = actions[laterActionIndex];
			const bool sameControlType =
			    (dynamic_cast<const MoveActionComponent*>(earlierAction) && dynamic_cast<const MoveActionComponent*>(laterAction)) ||
			    (dynamic_cast<const PlayAnimationActionComponent*>(earlierAction) && dynamic_cast<const PlayAnimationActionComponent*>(laterAction));
			if (sameControlType && earlierAction->TargetActor == laterAction->TargetActor &&
			    (earlierAction->Step == laterAction->Step || !earlierAction->WaitForCompletion))
			{
				myLastError = "Overlapping actions control the same movement or animation: " + laterAction->TargetActor;
				break;
			}
		}
	}

	if (!myLastError.empty())
	{
		std::cerr << "Sequence '" << GetOwner()->GetName() << "': " << myLastError << '\n';
		return false;
	}

	// All checks passed. Reset the actions for this run and reserve their actors before the first step starts.
	// Keep component names so later updates can look them up again if a component has been removed.
	myActionNames.clear();

	for (auto* action : actions)
	{
		action->Cleanup(true); // Stop any sound still playing from the previous run before starting it again.
		action->myStarted = false;
		action->myFinished = false;
		myActionNames.push_back(action->GetName());
		if (action->ClaimsTarget())
		{
			const Actor* targetActor = GetWorld().FindActor(action->TargetActor);
			actorClaims[targetActor] = this;
			myClaimedActors.push_back(targetActor);
		}
	}

	myRunning = true;
	myCompleted = false;
	myPlayed = true;
	myStepBeginIndex = 0;
	StartStep();
	return myRunning || myCompleted;
}

void ScriptedSequenceComponent::StartStep()
{
	// The sorted list keeps each step's actions together. Start this group and stop at the next step number.
	myStepEndIndex = myStepBeginIndex;
	const auto* firstActionInStep = dynamic_cast<ScriptedActionComponent*>(GetOwner()->FindComponent(myActionNames[myStepBeginIndex]));
	if (!firstActionInStep)
	{
		myLastError = "Action removed";
		Cancel();
		return;
	}
	const int currentStep = firstActionInStep->Step;
	while (myStepEndIndex < myActionNames.size())
	{
		auto* action = dynamic_cast<ScriptedActionComponent*>(GetOwner()->FindComponent(myActionNames[myStepEndIndex]));
		if (!action)
		{
			myLastError = "Action removed";
			Cancel();
			return;
		}
		if (action->Step != currentStep)
		{
			break;
		}
		// Mark it first so cancellation also cleans up anything a failed Start() may already have changed.
		action->myStarted = true;
		if (!action->Start())
		{
			myLastError = "Action could not start: " + action->GetName();
			Cancel();
			return;
		}
		++myStepEndIndex;
	}
}

void ScriptedSequenceComponent::Update(float deltaTime)
{
	if (!myRunning)
	{
		return;
	}
	// Check that the event still has its actions and targets before letting those actions use them.
	bool stepFinished = true;
	for (std::size_t actionIndex = 0; actionIndex < myActionNames.size(); ++actionIndex)
	{
		auto* action = dynamic_cast<ScriptedActionComponent*>(GetOwner()->FindComponent(myActionNames[actionIndex]));
		if (!action || !action->IsEnabled())
		{
			myLastError = "Action removed or disabled";
			Cancel();
			return;
		}
		if (!action->Validate(myLastError))
		{
			Cancel();
			return;
		}
		if (action->ClaimsTarget())
		{
			// An actor with the same name may have been replaced. Only continue with the actor we reserved at the start.
			const auto targetClaim = actorClaims.find(GetWorld().FindActor(action->TargetActor));
			if (targetClaim == actorClaims.end() || targetClaim->second != this)
			{
				myLastError = "Claimed target was replaced: " + action->TargetActor;
				Cancel();
				return;
			}
		}
		// Keep updating started actions even after their step has passed, such as a sound playing over later steps.
		if (action->myStarted && !action->myFinished)
		{
			action->myFinished = action->Tick(deltaTime);
		}
		// Only waiting actions in the current step hold up the next step.
		if (actionIndex >= myStepBeginIndex && actionIndex < myStepEndIndex && action->WaitForCompletion && !action->myFinished)
		{
			stepFinished = false;
		}
	}
	if (stepFinished)
	{
		// No steps left means the event ends, even if some non-waiting actions are still running.
		myStepBeginIndex = myStepEndIndex;
		if (myStepBeginIndex == myActionNames.size())
		{
			Finish(false);
		}
		else
		{
			StartStep();
		}
	}
}

void ScriptedSequenceComponent::ReleaseClaims() noexcept
{
	// Let other events use these actors again. Only remove reservations that still belong to this sequence.
	for (const Actor* claimedActor : myClaimedActors)
	{
		const auto actorClaim = actorClaims.find(claimedActor);
		if (actorClaim != actorClaims.end() && actorClaim->second == this)
		{
			actorClaims.erase(actorClaim);
		}
	}
	myClaimedActors.clear();
}

void ScriptedSequenceComponent::Finish(bool cancelled) noexcept
{
	// Give back temporary control such as movement, animation, and input locks.
	// Positions and changes to animation variables or enabled/active settings stay as the actions left them.
	for (const std::string& actionName : myActionNames)
	{
		if (auto* action = dynamic_cast<ScriptedActionComponent*>(GetOwner()->FindComponent(actionName)))
		{
			if (action->myStarted)
			{
				action->Cleanup(cancelled);
			}
		}
	}
	ReleaseClaims();
	myRunning = false;
	myCompleted = !cancelled;
}

void ScriptedSequenceComponent::Cancel() noexcept
{
	Finish(true);
}

void ScriptedSequenceComponent::EndPlay() noexcept
{
	// Unloading the scene or removing this component must also stop the event and disconnect its button listener.
	Cancel();
	if (myListeningForTrigger)
	{
		ServiceLocator::GetInstance().GetInputMapper().RemoveEventListener(myTriggerListener);
		myListeningForTrigger = false;
	}
}
