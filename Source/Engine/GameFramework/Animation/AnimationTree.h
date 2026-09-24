#pragma once
#include "AnimationState.h"

#include <GameFramework/Components/SkeletalMeshComponent.h>
//#include "AnimatedModel.h"

#include "CommonUtilities/Vector3.hpp"

#include <string>
#include <unordered_map>

struct AnimationVariable
{
	std::string Name;
	bool IsTrigger;

	bool IsActive;
};

class AnimationTree
{
public:
	friend class AnimationManager;
	friend class AnimationState;

	AnimationTree() : myAnimationPlayer(nullptr), myShouldUpdateAnimation(false), myCurrentState(0) {};
	AnimationTree(const std::string& aName, const std::string& aStartState, const std::vector<AnimationVariable>& someVariables);
	AnimationTree(const AnimationTree& aTree);
	~AnimationTree() = default;

	//void InitTree(AnimatedModel* aModel);

	// Only used for animation masking and syncing anims
	void InitTree(AnimationTree& aTree);

	void Update(const float aDeltaTime);

	const std::string& GetName();

	AnimationState& GetAnimationState(const std::string& aName);
	AnimationState& GetCurrentAnimationState();

	AnimationVariable& GetAnimationVariable(const std::string& aName);
	void SetBool(const std::string& aName, const bool aValue);
	void SetTrigger(const std::string& aName);

	void PlayState(const std::string& aName);

	SkeletalMeshComponent& GetAnimationPlayer();

private:
	void AddVariable(const AnimationVariable& anAnimationVariable);

	const std::unordered_map<std::string, AnimationVariable> GetVariables() const;
	const std::vector<AnimationState> GetStates() const;

	const std::string myName;
	const std::string myStartState;

	std::vector <Transition> myGlobalTransitions;
	void AddGlobalTransition(const Transition aTransition);

	SkeletalMeshComponent* myAnimationPlayer;
	bool myShouldUpdateAnimation;

	std::unordered_map<std::string, AnimationVariable> myAnimationVariables;

	std::vector<AnimationState> myAnimationStates;
	int myCurrentState;

};
