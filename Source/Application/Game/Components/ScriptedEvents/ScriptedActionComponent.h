#pragma once
#include "GameFramework/Components/CustomComponent.h"
#include "Vector3.hpp"
#include <string>

class ScriptedEventComponent;

// One job in an event, such as moving an actor or playing a sound.
// The sequence decides when it starts, updates it until it finishes, and cleans it up when the event ends.
class ScriptedActionComponent : public CustomComponent
{
public:
	// Equal step numbers start together. Higher numbers start after this step's waiting actions finish.
	int Step = 0;
	// False lets the sequence move on while this action continues running.
	bool WaitForCompletion = true;
	// Runtime actor/component names from the scene. These are not animation or sound asset paths.
	std::string TargetActor;
	std::string TargetComponent;

	void EndPlay() noexcept override
	{
		Cleanup(true);
	}

	// Check the setup without changing the scene. Start returns whether the action started successfully.
	virtual bool Validate(std::string& error) const;
	virtual bool Start() = 0;

	// Return true when done. Actions that make a one-time change use this default on their first update.
	virtual bool Tick(float)
	{
		return true;
	}

	// Give back temporary control. True means cancellation, removal, or preparing for replay; it also stops started audio.
	// Cleanup can be called more than once, so actions remember whether they still have anything to release.
	virtual void Cleanup(bool) noexcept
	{
	}

	// True asks the sequence to reserve this actor for the whole event. Waits and audio do not reserve actors.
	virtual bool ClaimsTarget() const
	{
		return !TargetActor.empty();
	}

	bool IsFinished() const
	{
		return myFinished;
	}

	bool IsStarted() const
	{
		return myStarted;
	}

protected:
	void ReadCommon(const CustomComponentVariables& variables);
	Actor* FindTarget() const;
	Component* FindTargetComponent() const;

private:
	friend class ScriptedEventComponent;
	bool myStarted = false;
	bool myFinished = false;
};

// Hold a step open for a number of seconds without changing an actor.
class WaitActionComponent final : public ScriptedActionComponent
{
public:
	float DurationInSeconds = 1.0f;
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Validate(std::string&) const override;
	bool Start() override;
	bool Tick(float) override;

	bool ClaimsTarget() const override
	{
		return false;
	}

private:
	float myElapsedSeconds = 0;
};

// Move an actor straight from its current position to a marker actor's position over DurationInSeconds seconds.
class MoveActionComponent final : public ScriptedActionComponent
{
public:
	std::string DestinationActor;
	float DurationInSeconds = 1.0f;
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Validate(std::string&) const override;
	bool Start() override;
	bool Tick(float) override;
	void Cleanup(bool) noexcept override;

private:
	CommonUtilities::Vector3f myStartPosition;
	CommonUtilities::Vector3f myDestinationPosition;
	float myElapsedSeconds = 0;
	bool myControlsMovement = false;
};

// Temporarily play a registered clip instead of letting the animation tree choose one.
class PlayAnimationActionComponent final : public ScriptedActionComponent
{
public:
	std::string Animation;
	bool Loop = false;
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Validate(std::string&) const override;
	bool Start() override;
	bool Tick(float) override;
	void Cleanup(bool) noexcept override;

private:
	bool myOverridesAnimation = false;
};

// Set a bool or trigger and let the existing animation tree respond to it.
class SetAnimationVariableActionComponent final : public ScriptedActionComponent
{
public:
	std::string Variable;
	bool Trigger = false;
	bool Value = true;
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Validate(std::string&) const override;
	bool Start() override;
};

// Start a registered sound, optionally at an actor's location. Tick finishes when that sound stops playing.
class PlayAudioActionComponent final : public ScriptedActionComponent
{
public:
	int Sound = 0;
	bool Positional = false;
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Validate(std::string&) const override;
	bool Start() override;
	bool Tick(float) override;
	void Cleanup(bool cancelled) noexcept override;

	bool ClaimsTarget() const override
	{
		return false;
	}

private:
	bool myStartedSound = false;
	bool mySkippedFirstCompletionCheck = false;
};

// Block player commands during the event. Cleanup releases a lock applied by this action.
class LockControlActionComponent final : public ScriptedActionComponent
{
public:
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Validate(std::string&) const override;
	bool Start() override;
	void Cleanup(bool) noexcept override;

private:
	bool myOwnsControlLock = false;
};

// Give player control back before the rest of the event finishes.
class UnlockControlActionComponent final : public ScriptedActionComponent
{
public:
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Validate(std::string&) const override;
	bool Start() override;
};

// Show/activate or hide/deactivate a whole actor, leaving that choice in place after the event.
class SetActorActiveActionComponent final : public ScriptedActionComponent
{
public:
	bool Active = true;
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Start() override;
};

// Turn one named component on or off, such as handing an enemy back to its normal behavior.
class SetComponentEnabledActionComponent final : public ScriptedActionComponent
{
public:
	bool Enabled = true;
	void InitializeCustomVariables(const CustomComponentVariables&) override;
	bool Validate(std::string&) const override;
	bool Start() override;
};
