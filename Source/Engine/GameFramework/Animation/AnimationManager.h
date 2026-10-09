#pragma once

#include "AnimationTree.h"

#include <filesystem>
#include <string>
#include <Vector.hpp>


class AnimationManager
{
public:

	struct Variable
	{
		std::string Name;
		std::string Type;
		bool Active = false;
	};

	struct DebugData
	{
		bool Enabled = false;
		std::string Mesh;
		std::string Tree;

		std::vector<Variable> Variables;
		CommonUtilities::Vector3<float> Position;
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
