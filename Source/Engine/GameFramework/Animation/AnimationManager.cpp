#include "AnimationManager.h"
#include "AnimationTree.h"
#include "AnimationState.h"
#include <iostream>

#include <GameFramework/SimdJson/simdjson.h>
#include <filesystem>
#include <fstream>

//#include <tge/settings/settings.h>

AnimationManager::AnimationManager()
{
	#ifndef _RETAIL // Enkel fix för när vi lägger om filer för inlämning
	std::string path = std::string("settings/AnimationTree.json");
	#else
	std::string path = std::string("settings/AnimationTree.json");  
	#endif

	const char* fileName = "AnimationTree.json";
	//std::string fullPath = Tga::Settings::ResolveAssetPath(fileName);

	std::ifstream file(path, std::ios::in);

	assert(file);

	simdjson::padded_string json = simdjson::padded_string::load(path.c_str());
	simdjson::dom::parser parser;
	simdjson::dom::object root;

	const auto& error = parser.parse(json).get(root);

	if (error)
	{
		return;
	}

	//file >> data;

	file.close();
	
	for (auto& tree : root)
	{
		//nlohmann::json& entity = tree.value();

		//nlohmann::json& variablesArray = entity["Variables"];
		//std::vector<AnimationVariable> variableNames;
		//for (auto& variables : variablesArray.items())
		//{
		//	bool isTrigger = false;

		//	if (variables.value()["Type"].get<std::string>() == "Trigger")
		//	{
		//		isTrigger = true;
		//	}

		//	variableNames.emplace_back(
		//	variables.value()["Name"].get<std::string>(),
		//	isTrigger,
		//	false
		//	);
		//}

		//entity.value("Name", "Default");
		//std::string name = entity["Name"].get<std::string>();
		//std::string startState = entity["Start state"].get<std::string>();
		//myAnimationTrees.emplace_back(std::move(name), startState, variableNames); // Creates a tree
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
