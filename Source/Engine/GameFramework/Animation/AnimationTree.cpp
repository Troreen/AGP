#include "AnimationTree.h"
#include "AnimationState.h"

#include <filesystem>
#include <fstream>
#include <cassert>
#include <unordered_map>
#include <GameFramework/SimdJson/simdjson.h>
#include <ServiceLocator.h>
#include <AssetHandling/AssetRegistry.h>
#include <GameFramework/Components/AnimatorComponent.h>

//#include <tge/model/ModelFactory.h>
//#include <tge/settings/settings.h>

AnimationTree::AnimationTree(const std::string& aName, const std::string& aStartState, const std::vector<AnimationVariable>& someVariables) : myName(aName), myStartState(aStartState)
{
	//myModel = nullptr;
	for (AnimationVariable variable : someVariables)
	{
		AddVariable(variable);
	}

	std::string file = static_cast<std::string>(aName.data()) + static_cast<std::string>(".json");

	AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();
	const std::filesystem::path& contentRoot = assetRegistry.GetContentRoot();
	std::filesystem::path treePath = contentRoot / "Animations" / "Animation Manager" / "Trees" / file;

	simdjson::padded_string json = simdjson::padded_string::load(treePath.c_str());
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
		std::string name(currentState["Name"].get<const char*>());
		std::string path(state.value["Animation"].get<const char*>());

		bool overwriteGlobal = false;
		if (!state.value["OverwriteGlobal"].error())
		{
			overwriteGlobal = state.value["OverwriteGlobal"].get<bool>();
		}

		myAnimationStates.emplace_back(name, path, overwriteGlobal); // Creates a state

		bool isLooping = true;
		bool isFullbody = true;

		if (!state.value["IsLooping"].error())
		{
			isLooping = state.value["IsLooping"].get<bool>();
		}
		//if (!state.value["IsBaseLayer"].error())
		//{
		//	isFullbody = state.value["IsBaseLayer"].get<bool>();
		//}



		myAnimationStates[myAnimationStates.size() - 1].SetAnimation(name, isLooping, isFullbody); // Gives the state essentials for animation

		if (!currentState["Global Transitions"].error())
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
				std::string(transitions["Variable"].get<const char*>()),
				transitions["Expected"].get<bool>()
				};

				AddGlobalTransition(newTransition);
			}
		}

		if (!currentState["Transitions"].error())
		{
			simdjson::dom::array transitionArray = currentState["Transitions"];
			for (auto transitions : transitionArray)
			{
				bool hasExitTime = false;

				if (!transitions["HasExitTime"].error())
				{
					hasExitTime = transitions["HasExitTime"].get<bool>();
				}

				Transition newTransition = {
				std::string(transitions["TransitionTo"].get<const char*>()),
				hasExitTime,
				std::string(transitions["Variable"].get<const char*>()),
				transitions["Expected"].get<bool>()
				};

				myAnimationStates[myAnimationStates.size() - 1].AddTransition(newTransition);
			}
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
void AnimationTree::InitTree()
{
	PlayState(myStartState);
}

AnimationVariable& AnimationTree::GetAnimationVariable(const std::string& aName)
{
	if (!myAnimationVariables.contains(aName))
	{
		std::cout << "No Animation Variable with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
		return myAnimationVariables.begin()->second;
	}

	return myAnimationVariables[aName];
}

void AnimationTree::SetBool(const std::string& aName, const bool aValue)
{
	if (!myAnimationVariables.contains(aName))
	{
		std::cout << "No Animation Bool with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
		return;
	}
	if (myAnimationVariables[aName].IsTrigger)
	{
		std::cout << "'" << aName << "' is not a bool in the tree '" << myName << "'!" << std::endl;
		return;
	}

	myAnimationVariables[aName].IsActive = aValue;
}

void AnimationTree::SetTrigger(const std::string& aName)
{
	if (!myAnimationVariables.contains(aName))
	{
		std::cout << "No Animation Trigger with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
		return;
	}
	if (!myAnimationVariables[aName].IsTrigger)
	{
		std::cout << "'" << aName << "' is not a trigger in the tree '" << myName << "'!" << std::endl;
		return;
	}

	myAnimationVariables[aName].IsActive = true;
}

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
	myAnimationStates[myCurrentState].Update(aDeltaTime, myGlobalTransitions);
	//if (myShouldUpdateAnimation)
	//{
	//	myAnimationPlayer->Update(aDeltaTime);
	//}
}

void AnimationTree::PlayState(const std::string& aName) // Used when we want to transition to new state
{
	for (int i = 0; i < myAnimationStates.size(); i++)
	{
		if (myAnimationStates[i].GetName() == aName)
		{
			myAnimationStates[myCurrentState].OnExit(); // Kan orsaka problem om state 0 har exit events, fast tbh om vi bara sätter events efter init är det kanske inte något att oroa sig om
			myCurrentState = i;
			myAnimationStates[myCurrentState].OnEnter();

			return;
		}
	}

	std::cout << "No AnimationState with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
}

void AnimationTree::SetAnimationPlayer(AnimatorComponent* aAnimationPlayer)
{
	myAnimationPlayer = aAnimationPlayer;

	for (auto& state : myAnimationStates)
	{
		state.myAnimationTree = this;

		std::shared_ptr<AnimationAsset> asset = ServiceLocator::GetInstance().GetAssetRegistry().GetAsset<AnimationAsset>(state.myPath);
		myAnimationPlayer->AddAnimation(state.myName, asset);
	}
}

AnimatorComponent* AnimationTree::GetAnimationPlayer()
{
	if (!myAnimationPlayer)
	{
		std::cout << "No Animation Player exists in " << myName << "!" << std::endl;
	}

	return myAnimationPlayer;
}

AnimationState& AnimationTree::GetAnimationState(const std::string& aName)
{
	for (int i = 0; i < myAnimationStates.size(); i++)
	{
		if (myAnimationStates[i].GetName() == aName)
		{
			return myAnimationStates[i];
		}
	}

	std::cout << "No AnimationState with the name '" << aName << "' exists in the tree '" << myName << "'!" << std::endl;
	return myAnimationStates[0];
}

AnimationState& AnimationTree::GetCurrentAnimationState()
{
	return myAnimationStates[myCurrentState];
}
