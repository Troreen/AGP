#pragma once
//#include "AnimatedModel.h"

#include <filesystem>
#include <string>

#include "AnimationTree.h"

class AnimationManager
{
public:
	AnimationManager();
	~AnimationManager() = default;

	void Initialize(const std::filesystem::path& aContentRoot);
	AnimationTree& GetAnimationTree(const std::string& aName);

private:
	std::vector<AnimationTree> myAnimationTrees;
};
