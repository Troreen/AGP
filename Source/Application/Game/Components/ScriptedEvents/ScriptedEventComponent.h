#pragma once
#include "GameFramework/Components/CustomComponent.h"
#include <string>
#include <vector>

class ScriptedActionComponent;

class ScriptedEventComponent final : public CustomComponent
{
public:
	// False allows one accepted start per scene load, even if that run is later cancelled.
	bool Replayable = false;
	// Optional input action for manual testing.
	std::string TriggerAction;
	// Optional collider on this Actor. An accepted collision start disables it.
	std::string TriggerCollider;
	void InitializeCustomVariables(const CustomComponentVariables& variables) override;
	void BeginPlay() override;
	void Update(float deltaTime) override;
	void OnCollisionEnter(const Collision& collision) override;
	void EndPlay() noexcept override;
	bool Play();
	void Cancel() noexcept;

	bool IsRunning() const { return myRunning; }

	bool HasCompleted() const { return myCompleted; }

	const std::string& GetLastError() const { return myLastError; }

private:
	void ReleaseClaims() noexcept;
	void Finish(bool cancelled) noexcept;
	void StartStep();
	// Names are stored in step order; actions sharing a step stay next to each other.
	std::vector<std::string> myActionNames;
	std::vector<const Actor*> myClaimedActors;
	// These are positions in myActionNames, not step numbers. The end index is the first action after the current step.
	std::size_t myStepBeginIndex = 0;
	std::size_t myStepEndIndex = 0;
	bool myRunning = false;
	bool myCompleted = false;
	bool myPlayed = false;
	unsigned myTriggerListener = 0;
	bool myListeningForTrigger = false;
	std::string myLastError;
};
