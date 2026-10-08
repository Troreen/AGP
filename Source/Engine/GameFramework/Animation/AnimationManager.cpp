#include "AnimationManager.h"
#include "AnimationTree.h"
#include "AnimationState.h"
#include <iostream>
#include <ServiceLocator.h>
#include <AssetHandling/AssetRegistry.h>
#include <Components/SkeletalMeshComponent.h>

#include <GameFramework/SimdJson/simdjson.h>
#include <filesystem>
#include <fstream>

AnimationManager::AnimationManager()
{
}

void AnimationManager::Initialize(const std::filesystem::path& aContentRoot)
{
	{
		std::filesystem::path path = aContentRoot / "Animations" / "Animation Manager" / "AnimationManager.json";
		simdjson::padded_string json = simdjson::padded_string::load(path.c_str());
		simdjson::dom::parser parser;
		simdjson::dom::object root;

		const auto& error = parser.parse(json).get(root);

		if (error)
		{
			return;
		}

		for (auto& t : root)
		{
			simdjson::dom::element tree = t.value;

			simdjson::dom::array variables = tree["Variables"].get_array();
			std::vector<AnimationVariable> variableNames;

			for (const auto& variable : variables)
			{
				bool isTrigger = false;

				if (variable["Type"].get<std::string_view>() == std::string_view("Trigger"))
				{
					isTrigger = true;
				}

				variableNames.emplace_back(
					std::string(variable["Name"].get<const char*>()),
					isTrigger,
					false
				);
			}

			//entity.value("Name", "Default");
			std::string name(tree["Name"].get<const char*>());
			std::string startState(tree["Start state"].get<const char*>());
			myAnimationTrees.emplace_back(name, startState, variableNames);
		}
	}

	{
		std::filesystem::path path = aContentRoot / "Animations" / "Animation Manager" / "AnimationDebug.json";

		simdjson::padded_string json = simdjson::padded_string::load(path.c_str());
		simdjson::dom::parser parser;
		simdjson::dom::object root;

		const auto& error = parser.parse(json).get(root);

		if (error)
		{
			return;
		}

		if (root["Debug Animation"]["Enabled"].get<bool>() == false)
		{
			return;
		}

		myDebugData.Mesh = root["Debug Animation"]["SK Mesh"].get<const char*>();
		myDebugData.Tree = root["Debug Animation"]["Tree"].get<const char*>();
	}

}

AnimationTree& AnimationManager::GetAnimationTree(const std::string& aName)
{
	for (int i = 0; i < myAnimationTrees.size(); i++)
	{
		if (myAnimationTrees[i].GetName() == aName)
		{
			return myAnimationTrees[i];
		}
	}

	std::cout << "No animation tree with the name '" << aName << "' exists!" << std::endl;
	return myAnimationTrees[0];

	//AnimationTree tree;
	//return tree;
}
