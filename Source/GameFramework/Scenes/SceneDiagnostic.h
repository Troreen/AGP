#pragma once
#include <string>
#include <vector>

struct SceneDiagnostic
{
    std::string Actor;
    std::string Component;
    std::string Property;
    std::string Message;
};
using SceneDiagnostics = std::vector<SceneDiagnostic>;
