#include "ScriptedActionComponent.h"
#include "GameFramework/Scenes/CustomComponentVariables.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/AnimatorComponent.h"
#include "GameFramework/ServiceLocator.h"
#include "GameFramework/AudioManager.h"
#include "Player/PlayerController.h"
#include "Player/PlayerMovementComponent.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	// A component name picks an exact animator/controller. Without a name, use the actor's first component of that type.
	template <class ComponentType> ComponentType* FindActionTargetComponent(const ScriptedActionComponent& action)
	{
		Actor* targetActor = action.GetWorld().FindActor(action.TargetActor);
		if (!targetActor)
		{
			return nullptr;
		}
		if (action.TargetComponent.empty())
		{
			return targetActor->GetComponent<ComponentType>();
		}
		return dynamic_cast<ComponentType*>(targetActor->FindComponent(action.TargetComponent));
	}

	// Save the reason an action cannot run and return false so its validation can stop here.
	bool SetValidationError(std::string& error, const std::string& message)
	{
		error = message;
		return false;
	}

	bool ValidDuration(float duration)
	{
		return std::isfinite(duration) && duration >= 0;
	}
}

void ScriptedActionComponent::ReadCommon(const CustomComponentVariables& variables)
{
	// These settings come from the exported custom component. Every action reads the same shared fields.
	// Check the step before fitting the exported 64-bit number into our int field.
	const auto exportedStep = variables.GetOrDefault<int64_t>("Step", 0);
	if (exportedStep < 0 || exportedStep > (std::numeric_limits<int>::max)())
	{
		throw std::invalid_argument("Invalid scripted action Step");
	}
	Step = static_cast<int>(exportedStep);
	WaitForCompletion = variables.GetOrDefault<bool>("WaitForCompletion", true);
	TargetActor = variables.GetOrDefault<std::string>("TargetActor", "");
	TargetComponent = variables.GetOrDefault<std::string>("TargetComponent", "");
}

Actor* ScriptedActionComponent::FindTarget() const
{
	// Look up the actor when needed rather than keeping a pointer that could outlive it.
	return GetWorld().FindActor(TargetActor);
}

Component* ScriptedActionComponent::FindTargetComponent() const
{
	Actor* targetActor = FindTarget();
	if (!targetActor)
	{
		return nullptr;
	}
	return targetActor->FindComponent(TargetComponent);
}

bool ScriptedActionComponent::Validate(std::string& error) const
{
	if (!FindTarget())
	{
		return SetValidationError(error, "Missing target Actor: " + TargetActor);
	}
	return true;
}

void WaitActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	Duration = variables.GetOrDefault<float>("Duration", 1.0f);
}

bool WaitActionComponent::Validate(std::string& error) const
{
	if (!ValidDuration(Duration))
	{
		return SetValidationError(error, "Invalid wait Duration");
	}
	return true;
}

bool WaitActionComponent::Start()
{
	myElapsedSeconds = 0;
	return true;
}

bool WaitActionComponent::Tick(float deltaTime)
{
	// Tick returns true when the action is done. A wait simply counts seconds until its duration is reached.
	myElapsedSeconds += deltaTime;
	return myElapsedSeconds >= Duration;
}

void MoveActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	DestinationActor = variables.GetOrDefault<std::string>("DestinationActor", "");
	Duration = variables.GetOrDefault<float>("Duration", 1.0f);
}

bool MoveActionComponent::Validate(std::string& error) const
{
	if (!ScriptedActionComponent::Validate(error))
	{
		return false;
	}
	if (!ValidDuration(Duration))
	{
		return SetValidationError(error, "Invalid move Duration");
	}
	if (!GetWorld().FindActor(DestinationActor))
	{
		return SetValidationError(error, "Missing destination Actor: " + DestinationActor);
	}
	return true;
}

bool MoveActionComponent::Start()
{
	// Save both positions once. Moving the marker later does not change this trip's destination.
	myStartPosition = FindTarget()->GetTransform().GetWorldPosition();
	myDestinationPosition = GetWorld().FindActor(DestinationActor)->GetTransform().GetWorldPosition();
	myElapsedSeconds = 0;
	myControlsMovement = true;
	// Pause normal player movement so it does not move the actor at the same time as this action.
	// Other movement or enemy behavior components must be disabled separately when authoring the event.
	if (auto* movement = FindTarget()->GetComponent<PlayerMovementComponent>())
	{
		movement->SetScriptedMovement(true);
	}
	return true;
}

bool MoveActionComponent::Tick(float deltaTime)
{
	// Progress goes from 0 (start) to 1 (destination). Zero duration moves there on the first update.
	// This is a straight-line position change; it does not find a path around obstacles or turn the actor.
	myElapsedSeconds += deltaTime;
	const float movementProgress = Duration == 0 ? 1.0f : (std::min)(myElapsedSeconds / Duration, 1.0f);
	FindTarget()->GetTransform().SetWorldPosition(myStartPosition + (myDestinationPosition - myStartPosition) * movementProgress);
	if (movementProgress >= 1)
	{
		Cleanup(false);
		return true;
	}
	return false;
}

void MoveActionComponent::Cleanup(bool) noexcept
{
	// Let normal movement run again, leaving the actor wherever the scripted move reached.
	if (myControlsMovement)
	{
		if (Actor* targetActor = FindTarget())
		{
			if (auto* movement = targetActor->GetComponent<PlayerMovementComponent>())
			{
				movement->SetScriptedMovement(false);
			}
		}
		myControlsMovement = false;
	}
}

void PlayAnimationActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	Animation = variables.GetOrDefault<std::string>("Animation", "");
	Loop = variables.GetOrDefault<bool>("Loop", false);
}

bool PlayAnimationActionComponent::Validate(std::string& error) const
{
	const auto* animator = FindActionTargetComponent<AnimatorComponent>(*this);
	if (!animator || !animator->CanBeginScriptedAnimation(Animation))
	{
		return SetValidationError(error, "Missing animator, mesh, or registered animation: " + Animation);
	}
	if (Loop && WaitForCompletion)
	{
		// A looping clip never finishes on its own, so waiting for it would keep this step open forever.
		return SetValidationError(error, "Looping animation cannot wait for completion");
	}
	return true;
}

bool PlayAnimationActionComponent::Start()
{
	// Play a clip already registered on the animator. The animation tree pauses while this clip has control.
	myOverridesAnimation = FindActionTargetComponent<AnimatorComponent>(*this)->BeginScriptedAnimation(Animation, Loop);
	return myOverridesAnimation;
}

bool PlayAnimationActionComponent::Tick(float)
{
	if (FindActionTargetComponent<AnimatorComponent>(*this)->IsScriptedAnimationPlaying())
	{
		return false;
	}
	Cleanup(false);
	return true;
}

void PlayAnimationActionComponent::Cleanup(bool) noexcept
{
	// Hand animation back to the tree's current clip without repeating its state entry events.
	// If there is no tree, the actor keeps its scripted pose until another animation is requested.
	if (myOverridesAnimation)
	{
		if (auto* animator = FindActionTargetComponent<AnimatorComponent>(*this))
		{
			animator->EndScriptedAnimation();
		}
		myOverridesAnimation = false;
	}
}

void SetAnimationVariableActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	Variable = variables.GetOrDefault<std::string>("Variable", "");
	Trigger = variables.GetOrDefault<bool>("Trigger", false);
	Value = variables.GetOrDefault<bool>("Value", true);
}

bool SetAnimationVariableActionComponent::Validate(std::string& error) const
{
	auto* animator = FindActionTargetComponent<AnimatorComponent>(*this);
	if (!animator || !animator->HasAnimationVariable(Variable))
	{
		return SetValidationError(error, "Missing animation-tree variable: " + Variable);
	}
	if (animator->GetTree().GetAnimationVariable(Variable).IsTrigger != Trigger)
	{
		return SetValidationError(error, "Animation variable kind does not match Trigger: " + Variable);
	}
	return true;
}

bool SetAnimationVariableActionComponent::Start()
{
	// Change the tree's settings and finish immediately. The tree decides which animation to play on its next update.
	// A bool stores on/off; a trigger is consumed when the tree uses it for a transition.
	auto& animationTree = FindActionTargetComponent<AnimatorComponent>(*this)->GetTree();
	if (Trigger)
	{
		animationTree.SetTrigger(Variable);
	}
	else
	{
		animationTree.SetBool(Variable, Value);
	}
	return true;
}

void PlayAudioActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	const auto soundID = variables.GetOrDefault<int64_t>("Sound", 0);
	if (soundID < eMainTheme || soundID >= eSoundCount)
	{
		throw std::invalid_argument("Unknown scripted Sound ID");
	}
	Sound = static_cast<int>(soundID);
	Positional = variables.GetOrDefault<bool>("Positional", false);
}

bool PlayAudioActionComponent::Validate(std::string& error) const
{
	if (Sound < eMainTheme || Sound >= eSoundCount)
	{
		return SetValidationError(error, "Unknown scripted Sound ID");
	}
	if (Positional)
	{
		return ScriptedActionComponent::Validate(error);
	}
	return true;
}

bool PlayAudioActionComponent::Start()
{
	// Positional audio starts at this location and stays there; it does not follow the actor.
	// Each SoundID uses one registered playback, so actions using the same ID share control of that sound.
	CommonUtilities::Vector3f soundPosition;
	if (Positional)
	{
		soundPosition = FindTarget()->GetTransform().GetWorldPosition();
	}
	myStartedSound =
	    ServiceLocator::GetInstance().GetAudioManager().PlayScriptedAudio(static_cast<SoundID>(Sound), Positional ? &soundPosition : nullptr);
	mySkippedFirstCompletionCheck = false;
	return myStartedSound;
}

bool PlayAudioActionComponent::Tick(float)
{
	// FMOD may not report the sound as playing immediately. Skip our first check so it has a chance to start.
	if (!mySkippedFirstCompletionCheck)
	{
		mySkippedFirstCompletionCheck = true;
		return false;
	}
	return !ServiceLocator::GetInstance().GetAudioManager().IsEventPlaying(static_cast<SoundID>(Sound));
}

void PlayAudioActionComponent::Cleanup(bool cancelled) noexcept
{
	// A sound may continue after the sequence finishes. Stop it on cancellation,
	// replay or scene unload, while leaving other registered sounds alone.
	if (myStartedSound && cancelled)
	{
		ServiceLocator::GetInstance().GetAudioManager().StopScriptedAudio(static_cast<SoundID>(Sound));
		myStartedSound = false;
	}
}

void LockControlActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	if (TargetActor.empty())
	{
		TargetActor = "Player";
	}
}

bool LockControlActionComponent::Validate(std::string& error) const
{
	if (!FindActionTargetComponent<PlayerController>(*this))
	{
		return SetValidationError(error, "Missing player controller: " + TargetActor);
	}
	return true;
}

bool LockControlActionComponent::Start()
{
	// Remember whether we applied the lock. Cleanup must not undo a lock that was already there before this action.
	auto* controller = FindActionTargetComponent<PlayerController>(*this);
	myOwnsControlLock = !controller->IsScriptedControlLocked();
	controller->SetScriptedControlLocked(true);
	return true;
}

void LockControlActionComponent::Cleanup(bool) noexcept
{
	if (myOwnsControlLock)
	{
		if (auto* controller = FindActionTargetComponent<PlayerController>(*this))
		{
			controller->SetScriptedControlLocked(false);
		}
		myOwnsControlLock = false;
	}
}

void UnlockControlActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	if (TargetActor.empty())
	{
		TargetActor = "Player";
	}
}

bool UnlockControlActionComponent::Validate(std::string& error) const
{
	if (!FindActionTargetComponent<PlayerController>(*this))
	{
		return SetValidationError(error, "Missing player controller: " + TargetActor);
	}
	return true;
}

bool UnlockControlActionComponent::Start()
{
	// Release this event's lock actions for the chosen controller, allowing gameplay to resume before the event ends.
	const auto* controller = FindActionTargetComponent<PlayerController>(*this);
	for (auto* lockAction : GetOwner()->GetComponents<LockControlActionComponent>())
	{
		if (FindActionTargetComponent<PlayerController>(*lockAction) == controller)
		{
			lockAction->Cleanup(false);
		}
	}
	return true;
}

void SetActorActiveActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	Active = variables.GetOrDefault<bool>("Active", true);
}

bool SetActorActiveActionComponent::Start()
{
	// This change stays after the event. An inactive actor stops updating and is hidden.
	FindTarget()->SetActive(Active);
	return true;
}

void SetComponentEnabledActionComponent::InitializeCustomVariables(const CustomComponentVariables& variables)
{
	ReadCommon(variables);
	Enabled = variables.GetOrDefault<bool>("Enabled", true);
}

bool SetComponentEnabledActionComponent::Validate(std::string& error) const
{
	if (!FindTargetComponent())
	{
		return SetValidationError(error, "Missing target component: " + TargetActor + "/" + TargetComponent);
	}
	return true;
}

bool SetComponentEnabledActionComponent::Start()
{
	// Change only the named component, for example to start enemy behavior while keeping the actor visible.
	// This setting stays after the event ends.
	FindTargetComponent()->SetEnabled(Enabled);
	return true;
}
