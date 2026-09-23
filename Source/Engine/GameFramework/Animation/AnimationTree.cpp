//#include "AnimationTree.h"
//#include "AnimationState.h"
//
//#include <filesystem>
//#include <fstream>
//#include <cassert>
//#include <unordered_map>
//#include <nlohmann/json.hpp>
//#include <tge/model/ModelFactory.h>
//#include <tge/settings/settings.h>
//
//AnimationTree::AnimationTree(const std::string& aName, const std::string& aStartState, const std::vector<AnimationVariable>& someVariables) : myName(aName), myStartState(aStartState)
//{
//	//myModel = nullptr;
//	for (AnimationVariable variable : someVariables)
//	{
//		AddVariable(variable);
//	}
//
//
//#ifndef _RETAIL
//	std::string path = std::string("settings/Animation Trees/" + aName + ".json");
//#else
//	std::string path = std::string("settings/Animation Trees/" + aName + ".json");
//#endif
//
//	nlohmann::json data;
//
//	std::ifstream file(path, std::ios::in);
//
//	assert(file);
//	file >> data; // If the name in the "AnimationTree" json file doensn't match the name of the json file in the "Animation Trees" folder, this will crash.
//
//	file.close();
//
//	for (auto& state : data.items())
//	{
//		nlohmann::json& currentState = state.value();
//		currentState.value("Name", "Default");
//		std::string name = currentState["Name"].get<std::string>();
//		
//		bool overwriteGlobal = false;
//		if (state.value().contains("OverwriteGlobal"))
//		{
//			overwriteGlobal = state.value()["OverwriteGlobal"].get<bool>();
//		}
//
//		myAnimationStates.emplace_back(name, overwriteGlobal); // Creates a state
//
//		bool isLooping = true;
//		bool isFullbody = true;
//
//		if (state.value().contains("IsLooping"))
//		{
//			isLooping = state.value()["IsLooping"].get<bool>();
//		}
//		if (state.value().contains("IsFullBody"))
//		{
//			isFullbody = state.value()["IsFullBody"].get<bool>();
//		}
//
//		myAnimationStates[myAnimationStates.size() - 1].SetAnimation(state.value()["Animation"].get<std::string>(), isLooping, isFullbody); // Gives the state essentials for animation
//
//		if (state.value().contains("IsGlobal"))
//		{
//			if (state.value()["IsGlobal"].get<bool>())
//			{
//				nlohmann::json& transitionArray = currentState["Global Transitions"];
//				for (auto& transitions : transitionArray.items())
//				{
//					bool hasExitTime = false;
//
//					if (state.value().contains("HasExitTime"))
//					{
//						hasExitTime = state.value()["HasExitTime"].get<bool>();
//					}
//
//					Transition newTransition = {
//					name,
//					hasExitTime,
//					transitions.value()["Variable"].get<std::string>(),
//					transitions.value()["Expected"].get<bool>()
//					};
//
//					AddGlobalTransition(newTransition);
//				}
//			}
//		}
//
//		nlohmann::json& transitionArray = currentState["Transitions"];
//		for (auto& transitions : transitionArray.items())
//		{
//			bool hasExitTime = false;
//
//			if (transitions.value().contains("HasExitTime"))
//			{
//				hasExitTime = transitions.value()["HasExitTime"].get<bool>();
//			}
//
//			Transition newTransition = {
//			transitions.value()["TransitionTo"].get<std::string>(),
//			hasExitTime,
//			transitions.value()["Variable"].get<std::string>(),
//			transitions.value()["Expected"].get<bool>()
//			};
//
//			myAnimationStates[myAnimationStates.size() - 1].AddTransition(newTransition);
//		}
//	}
//}
//
//AnimationTree::AnimationTree(const AnimationTree& aTree) : myName(aTree.myName), myStartState(aTree.myStartState)
//{
//	myAnimationStates = aTree.GetStates();
//	myAnimationVariables = aTree.GetVariables();
//	myGlobalTransitions = aTree.myGlobalTransitions;
//}
//
//void AnimationTree::AddVariable(const AnimationVariable& anAnimationVariable)
//{
//	myAnimationVariables.insert({ anAnimationVariable.Name, anAnimationVariable });
//}
//
//void AnimationTree::InitTree(AnimatedModel* aModel)
//{
//	myAnimationPlayer = new CoolAnimationPlayer();
//	myAnimationPlayer->SetModel(aModel);
//	myShouldUpdateAnimation = true;
//
//	for (AnimationState& state : myAnimationStates)
//	{
//		Tga::ModelFactory::GetInstance().GetAnimation(state.GetAnimationPath(), aModel->GetAnimatedModelInstance().GetModel());
//		state.InitState(this);
//	}
//
//	PlayState(myStartState);
//}
//
//void AnimationTree::InitTree(AnimationTree& aTree)
//{
//	myAnimationPlayer = aTree.myAnimationPlayer;
//	myShouldUpdateAnimation = false;
//
//	for (AnimationState& state : myAnimationStates)
//	{
//		state.InitState(this);
//	}
//
//	PlayState(myStartState);
//}
//
//AnimationVariable& AnimationTree::GetAnimationVariable(const std::string& aName)
//{
//	if (!myAnimationVariables.contains(aName))
//	{
//		std::cout << "No Animation Variable with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
//		return myAnimationVariables.begin()->second;
//	}
//
//	return myAnimationVariables[aName];
//}
//
//void AnimationTree::SetBool(const std::string& aName, const bool aValue)
//{
//	if (!myAnimationVariables.contains(aName))
//	{
//		std::cout << "No Animation Bool with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
//		return;
//	}
//	if (myAnimationVariables[aName].IsTrigger)
//	{
//		std::cout << "'" << aName << "' is not a bool in the tree '" << myName << "'!" << std::endl;
//		return;
//	}
//
//	myAnimationVariables[aName].IsActive = aValue;
//}
//
//void AnimationTree::SetTrigger(const std::string& aName)
//{
//	if (!myAnimationVariables.contains(aName))
//	{
//		std::cout << "No Animation Trigger with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
//		return;
//	}
//	if (!myAnimationVariables[aName].IsTrigger)
//	{
//		std::cout << "'" << aName << "' is not a trigger in the tree '" << myName << "'!" << std::endl;
//		return;
//	}
//
//	myAnimationVariables[aName].IsActive = true;
//}
//
//const std::unordered_map<std::string, AnimationVariable> AnimationTree::GetVariables() const
//{
//	return myAnimationVariables;
//}
//
//const std::vector<AnimationState> AnimationTree::GetStates() const
//{
//	return myAnimationStates;
//}
//
//
//void AnimationTree::AddGlobalTransition(const Transition aTransition)
//{
//	myGlobalTransitions.emplace_back(aTransition);
//}
//
//const std::string& AnimationTree::GetName()
//{
//	return myName;
//}
//
//void AnimationTree::Update(const float aDeltaTime)
//{
//	myAnimationStates[myCurrentState].Update(aDeltaTime, myGlobalTransitions);
//	if (myShouldUpdateAnimation)
//	{
//		myAnimationPlayer->Update(aDeltaTime);
//	}
//}
//
//void AnimationTree::PlayState(const std::string& aName) // Used when we want to transition to new state
//{
//	for (int i = 0; i < myAnimationStates.size(); i++)
//	{
//		if (myAnimationStates[i].GetName() == aName)
//		{
//			myAnimationStates[myCurrentState].OnExit(); // Kan orsaka problem om state 0 har exit events, fast tbh om vi bara sätter events efter init är det kanske inte något att oroa sig om
//			myCurrentState = i;
//			myAnimationStates[myCurrentState].OnEnter();
//
//			return;
//		}
//	}
//
//	std::cout << "No AnimationState with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
//}
//
//CoolAnimationPlayer& AnimationTree::GetAnimationPlayer()
//{
//	return *myAnimationPlayer;
//}
//
//AnimationState& AnimationTree::GetAnimationState(const std::string& aName)
//{
//	for (int i = 0; i < myAnimationStates.size(); i++)
//	{
//		if (myAnimationStates[i].GetName() == aName)
//		{
//			return myAnimationStates[i];
//		}
//	}
//
//	std::cout << "No AnimationState with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
//	return myAnimationStates[0];
//}
//
//AnimationState& AnimationTree::GetCurrentAnimationState()
//{
//	return myAnimationStates[myCurrentState];
//}
