#pragma once

#include "AnimationTree.h"

#include <filesystem>
#include <string>


class AnimationManager
{
public:
	struct DebugData
	{
		std::string Mesh;
		std::string Tree;
	};

	AnimationManager();
	~AnimationManager() = default;

	void Initialize(const std::filesystem::path& aContentRoot);
	AnimationTree& GetAnimationTree(const std::string& aName);
	DebugData& GetDebugData() { return myDebugData; }

private:
	std::vector<AnimationTree> myAnimationTrees;

	DebugData myDebugData;
};
