//#include "AnimationState.h"
//#include "AnimationTree.h"
//#include <tge/model/ModelFactory.h>
//
//AnimationState::AnimationState()
//{
//}
//
//AnimationState::AnimationState(const std::string& aName, bool aOverwriteGlobal) : myName(aName), myOverwriteGlobal(aOverwriteGlobal)
//{
//	myAnimationTree = nullptr;
//}
//
//AnimationState::~AnimationState()
//{
//	myAnimationTree = nullptr;
//}
//
//
//void AnimationState::SetAnimation(const std::string& aFilePath, bool aIsLooping, bool aIsFullBody)
//{
//	myFilePath = aFilePath;
//	myIsLooping = aIsLooping;
//
//	if (aIsFullBody)
//	{
//		myLayer = eAnimationLayer::FullBody;
//	}
//	else
//	{
//		myLayer = eAnimationLayer::UpperBody;
//	}
//}
//
//void AnimationState::InitState(AnimationTree* anAnimationTree)
//{
//	myAnimationTree = anAnimationTree;
//}
//
//void AnimationState::AddTransition(const Transition aTransition)
//{
//	myTransitions.emplace_back(aTransition);
//}
//
//void AnimationState::OnEnter()
//{
//	myOnEnter.Invoke();
//
//	myAnimationTree->myAnimationPlayer->SetAnimation(myFilePath, myIsLooping, myLayer);
//	myAnimationTree->myAnimationPlayer->Play(myLayer);
//}
//
//void AnimationState::Update(const float aDeltaTime, std::vector<Transition> someGlobalTransitions)
//{
//	myUpdate.Invoke(aDeltaTime);
//
//	if (!someGlobalTransitions.empty() && !myOverwriteGlobal) // If there are global transitions, they will be checked first
//	{
//		if (TransitionCheck(someGlobalTransitions))
//		{
//			return;
//		}
//	}
//	
//	TransitionCheck(myTransitions);
//}
//
//void AnimationState::OnExit()
//{
//	myOnExit.Invoke();
//
//	myAnimationTree->myAnimationPlayer->Stop(myLayer);
//}
//
//Event<>& AnimationState::OnEnterEvent()
//{
//	return myOnEnter;
//}
//
//Event<float>& AnimationState::UpdateEvent()
//{
//	return myUpdate;
//}
//
//Event<>& AnimationState::OnExitEvent()
//{
//	return myOnExit;
//}
//
//AnimationState& AnimationState::operator=(const AnimationState& other)
//{
//	if (this == &other)
//	{
//		return *this;
//	}
//
//	myName = other.myName;
//	myAnimationTree = other.myAnimationTree;
//
//	myTransitions.clear();
//	for (size_t i = 0; i < other.myTransitions.size(); i++)
//	{
//		myTransitions.emplace_back(other.myTransitions[i]);
//	}
//	
//	myFilePath = other.myFilePath;
//	myIsLooping = other.myIsLooping;
//	myLayer = other.myLayer;
//
//	return *this;
//}
//
//const std::string AnimationState::GetAnimationPath() const
//{
//	return myFilePath;
//}
//
//const std::string& AnimationState::GetName() const
//{
//	return myName;
//}
//
//bool AnimationState::TransitionCheck(const std::vector<Transition>& someTransitions)
//{
//	for (Transition transition : someTransitions)
//	{
//		if (transition.TransitionState == myName)
//		{
//			continue;
//		}
//
//		AnimationVariable& variable = myAnimationTree->GetAnimationVariable(transition.VariableName);
//
//		if (variable.IsActive == transition.Expected)
//		{
//			if (transition.HasExitTime)
//			{
//				if (myIsLooping)
//				{
//					if (!myAnimationTree->myAnimationPlayer->IsLoopFinished(myLayer))
//					{
//						continue;
//					}
//				}
//				else
//				{
//					if (!myAnimationTree->myAnimationPlayer->IsFinished(myLayer))
//					{
//						continue;
//					}
//				}
//			}
//
//			if (variable.IsTrigger)
//			{
//				variable.IsActive = false;
//			}
//
//			myAnimationTree->PlayState(transition.TransitionState);
//
//			return true;
//		}
//	}
//
//	return false;
//}
