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
#ifndef _RETAIL
	std::filesystem::path path = aContentRoot / "Animations" /"Animation Manager" / "AnimationManager.json";
#else
	std::filesystem::path path = aContentRoot / "Animations" /"Animation Manager" / "AnimationManager.json";
#endif

	//const char* fileName = "AnimationTree.json";
	//std::string fullPath = Tga::Settings::ResolveAssetPath(fileName);

	//std::ifstream file(path, std::ios::in);

	//assert(file);

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
