#pragma once
#include <string>
#include <vector>

struct SourceLocation
{
    std::string File, Object, Component, Field;
};

struct SceneDiagnostic
{
    std::string Actor;
    std::string Component;
    std::string Property;
    std::string Message;
    std::string File;
    std::string Code;
    std::string Phase;
    std::string Type;
};
using SceneDiagnostics = std::vector<SceneDiagnostic>;
