#pragma once
#include "../Helper/Event.hpp"

#include <string>

//#include <CoolAnimationPlayer.h>
#include <vector>

struct Transition
{
	std::string TransitionState;

	bool HasExitTime;

	std::string VariableName;
	bool Expected;
};

enum class eAnimationLayer
{
	BaseLayer,
	PartialLayer

};

class AnimationState
{
	friend class AnimationTree;

public:
	AnimationState();
	AnimationState(const std::string& aName, const std::string& aPath, bool aOverwriteGlobal);
	~AnimationState();

	void InitState(AnimationTree* anAnimationTree);

	Event<>& OnEnterEvent();
	Event<float>& UpdateEvent();
	Event<>& OnExitEvent();

	AnimationState& operator=(const AnimationState& other);

	const std::string GetAnimationPath() const;
	const std::string& GetName() const;

private:
	void SetAnimation(const std::string& aFilePath, bool aIsLooping = false, bool aIsFullBody = true);

	void AddTransition(const Transition aTransition);

	void OnEnter();
	void Update(const float aDeltaTime, std::vector<Transition> someGlobalTransitions = {});
	void OnExit();
	bool TransitionCheck(const std::vector<Transition>& someTransitions);

	AnimationTree* myAnimationTree;

	eAnimationLayer myLayer;
	bool myOverwriteGlobal;

	std::vector <Transition> myTransitions;

	Event<> myOnEnter;
	Event<float> myUpdate;
	Event<> myOnExit;

	std::string myName;
	std::string myPath;
	bool myIsLooping;
};
