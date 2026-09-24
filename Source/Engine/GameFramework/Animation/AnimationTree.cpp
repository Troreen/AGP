#include "AnimationTree.h"
#include "AnimationState.h"

#include <filesystem>
#include <fstream>
#include <cassert>
#include <unordered_map>
#include <GameFramework/SimdJson/simdjson.h>
//#include <tge/model/ModelFactory.h>
//#include <tge/settings/settings.h>

AnimationTree::AnimationTree(const std::string& aName, const std::string& aStartState, const std::vector<AnimationVariable>& someVariables) : myName(aName), myStartState(aStartState)
{
	//myModel = nullptr;
	for (AnimationVariable variable : someVariables)
	{
		AddVariable(variable);
	}

	std::string file = aName + ".json";

#ifndef _RETAIL
	std::filesystem::path path = std::filesystem::path().root_directory() / "Animation Trees" / file;
#else
	std::filesystem::path path = std::filesystem::path().root_directory() / "Animation Trees" / file;
#endif

	simdjson::padded_string json = simdjson::padded_string::load(path.c_str());
	simdjson::dom::parser parser;
	simdjson::dom::object root;

	const auto& error = parser.parse(json).get(root);

	if (error)
	{
		return;
	}

	//std::ifstream file(path, std::ios::in);

	//assert(file);
	//file >> json; // If the name in the "AnimationTree" json file doensn't match the name of the json file in the "Animation Trees" folder, this will crash.

	//file.close();

	for (auto& state : root)
	{
		simdjson::dom::element currentState = state.value;

		//currentState["Name", "Default"]; // Why here?
		std::string_view name = currentState["Name"].get<std::string_view>();

		bool overwriteGlobal = false;
		if (!state.value["OverwriteGlobal"].error())
		{
			overwriteGlobal = state.value["OverwriteGlobal"].get<bool>();
		}

		myAnimationStates.emplace_back(name, overwriteGlobal); // Creates a state

		bool isLooping = true;
		bool isFullbody = true;

		if (!state.value["IsLooping"].error())
		{
			isLooping = state.value["IsLooping"].get<bool>();
		}
		if (!state.value["IsFullBody"].error())
		{
			isFullbody = state.value["IsFullBody"].get<bool>();
		}

		myAnimationStates[myAnimationStates.size() - 1].SetAnimation(state.value["Animation"].get<std::string_view>(), isLooping, isFullbody); // Gives the state essentials for animation

		if (!state.value["IsGlobal"].error())
		{
			if (state.value["IsGlobal"].get<bool>())
			{
				simdjson::dom::array transitionArray = currentState["Global Transitions"];
				for (auto transitions : transitionArray)
				{
					bool hasExitTime = false;

					if (!state.value["HasExitTime"].error())
					{
						hasExitTime = state.value["HasExitTime"].get<bool>();
					}

					Transition newTransition = {
					name,
					hasExitTime,
					transitions["Variable"].get<std::string_view>(),
					transitions["Expected"].get<bool>()
					};

					AddGlobalTransition(newTransition);
				}
			}
		}

		simdjson::dom::array transitionArray = currentState["Transitions"];
		for (auto transitions : transitionArray)
		{
			bool hasExitTime = false;

			if (!transitions["HasExitTime"].error())
			{
				hasExitTime = transitions["HasExitTime"].get<bool>();
			}

			Transition newTransition = {
			transitions["TransitionTo"].get<std::string_view>(),
			hasExitTime,
			transitions["Variable"].get<std::string_view>(),
			transitions["Expected"].get<bool>()
			};

			myAnimationStates[myAnimationStates.size() - 1].AddTransition(newTransition);
		}
	}
}

AnimationTree::AnimationTree(const AnimationTree& aTree) : myName(aTree.myName), myStartState(aTree.myStartState)
{
	myAnimationStates = aTree.GetStates();
	myAnimationVariables = aTree.GetVariables();
	myGlobalTransitions = aTree.myGlobalTransitions;
}

void AnimationTree::AddVariable(const AnimationVariable& anAnimationVariable)
{
	myAnimationVariables.insert({ anAnimationVariable.Name, anAnimationVariable });
}
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
const std::unordered_map<std::string, AnimationVariable> AnimationTree::GetVariables() const
{
	return myAnimationVariables;
}

const std::vector<AnimationState> AnimationTree::GetStates() const
{
	return myAnimationStates;
}


void AnimationTree::AddGlobalTransition(const Transition aTransition)
{
	myGlobalTransitions.emplace_back(aTransition);
}

const std::string& AnimationTree::GetName()
{
	return myName;
}

void AnimationTree::Update(const float aDeltaTime)
{
	aDeltaTime;
	//myAnimationStates[myCurrentState].Update(aDeltaTime, myGlobalTransitions);
	//if (myShouldUpdateAnimation)
	//{
	//	myAnimationPlayer->Update(aDeltaTime);
	//}
}
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
