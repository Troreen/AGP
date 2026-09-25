#pragma once
#include "../Helper/Event.hpp"

#include <string>

//#include <CoolAnimationPlayer.h>
#include <vector>

struct Transition
{
	std::string_view TransitionState;

	bool HasExitTime;

	std::string_view VariableName;
	bool Expected;
};

class AnimationState
{
	friend class AnimationTree;

public:
	AnimationState();
	AnimationState(const std::string_view& aName, bool aOverwriteGlobal);
	~AnimationState();

	void InitState(AnimationTree* anAnimationTree);

	Event<>& OnEnterEvent();
	Event<float>& UpdateEvent();
	Event<>& OnExitEvent();

	AnimationState& operator=(const AnimationState& other);

	const std::string GetAnimationPath() const;
	const std::string& GetName() const;

private:
	void SetAnimation(const std::string_view& aFilePath, bool aIsLooping = false, bool aIsFullBody = true);

	void AddTransition(const Transition aTransition);

	void OnEnter();
	void Update(const float aDeltaTime, std::vector<Transition> someGlobalTransitions = {});
	void OnExit();
	bool TransitionCheck(const std::vector<Transition>& someTransitions);

	std::string myName;
	AnimationTree* myAnimationTree;

	//eAnimationLayer myLayer;
	bool myOverwriteGlobal;

	std::vector <Transition> myTransitions;

	Event<> myOnEnter;
	Event<float> myUpdate;
	Event<> myOnExit;

	std::string myFilePath;
	bool myIsLooping;
};
