#pragma once
#include "SceneDiagnostic.h"
#include <memory>
#include <optional>
#include <string>
namespace GameFrameworkInternal { struct SessionState; struct SceneServiceState; class SceneServiceAccess; }
struct SceneId
{
    std::string Value;
    bool operator==(const SceneId&) const = default;
};
enum class SceneLoadStatus { Idle, Requested, Loading, Loaded, Failed };
struct SceneLoadError
{
    SceneId Scene;
    SceneDiagnostics Diagnostics;
};
// Requests are deferred until the next host boundary; the latest request wins.
// A rejected replacement preserves the current world and its references.
class SceneService
{
public:
    ~SceneService();
    SceneService(const SceneService&) = delete;
    SceneService& operator=(const SceneService&) = delete;
    bool Load(SceneId id);
    bool Reload();
    SceneLoadStatus GetStatus() const;
    std::optional<SceneId> GetCurrent() const;
    std::optional<SceneLoadError> GetLastError() const;
private:
    SceneService();
    std::unique_ptr<GameFrameworkInternal::SceneServiceState> myState;
    friend struct GameFrameworkInternal::SessionState;
    friend class GameFrameworkInternal::SceneServiceAccess;
};
