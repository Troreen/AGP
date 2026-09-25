#include "AnimationManager.h"
#include "AnimationTree.h"
#include "AnimationState.h"
#include <iostream>
#include <ServiceLocator.h>
#include <AssetHandling/AssetRegistry.h>

#include <GameFramework/SimdJson/simdjson.h>
#include <filesystem>
#include <fstream>


AnimationManager::AnimationManager()
{
	AssetRegistry& assetRegistry = ServiceLocator::GetInstance().GetAssetRegistry();
	const std::filesystem::path& contentRoot = assetRegistry.GetContentRoot();
#ifndef _RETAIL
	std::filesystem::path path = contentRoot / "Animations" /"Animation Manager" / "AnimationManager.json";
#else
	std::filesystem::path path = contentRoot / "Animations" /"Animation Manager" / "AnimationManager.json";
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

			if (variable["Type"].get_c_str() == "Trigger")
			{
				isTrigger = true;
			}

			variableNames.emplace_back(
			variable["Name"].get<std::string_view>(),
			isTrigger,
			false
			);
		}

		//entity.value("Name", "Default");
		std::string_view name = tree["Name"].get<std::string_view>();
		std::string_view startState = tree["Start state"].get<std::string_view>();
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
