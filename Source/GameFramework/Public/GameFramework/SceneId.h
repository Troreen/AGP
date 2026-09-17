#pragma once
#include <string>

class SceneId
{
public:
	std::string Value;
	bool operator==(const SceneId&) const = default;
};
