#pragma once
#include <string>
struct SceneId
{
    std::string Value;
    bool operator==(const SceneId&) const = default;
};
