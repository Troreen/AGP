#pragma once
#include <filesystem>
#include <memory>
#include "Vector2.hpp"

class World;
class CameraComponent;
struct GameInput;
class SceneService;
class AssetLookup;
class GameTime;
namespace GameFrameworkInternal { struct SessionState; }

// A borrowed view of the session. Callbacks are serialized but may use different
// OS threads. Borrow world/input only within a callback; retain object refs instead.
class GameContext
{
public:
    ~GameContext();
    GameContext(const GameContext&) = delete;
    GameContext& operator=(const GameContext&) = delete;
    World& GetWorld();
    const GameInput& GetInput() const;
    SceneService& GetScenes();
    const AssetLookup& GetAssets() const;
    const GameTime& GetTime() const;
    const std::filesystem::path& GetContentRoot() const;
    // Initial client size; resize propagation is not implemented.
    CommonUtilities::Vector2u GetClientSize() const;
    void RequestQuit();
private:
    GameContext();
    std::unique_ptr<GameFrameworkInternal::SessionState> myState;
    friend class GameApplication;
};
