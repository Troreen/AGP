#pragma once
#include <filesystem>
#include <memory>
#include "Vector2.hpp"

class World;
class CameraComponent;
struct GameInput;
namespace GameFrameworkInternal { struct SessionState; }
namespace GameFrameworkIntegration { class LegacySceneBridge; }

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
    const std::filesystem::path& GetContentRoot() const;
    // Initial client size; resize propagation is not implemented.
    CommonUtilities::Vector2u GetClientSize() const;
    // Temporary convenience alias; prefer GetWorld().SetActiveCamera(camera).
    void SetActiveCamera(CameraComponent* camera);
    void RequestQuit();
private:
    GameContext();
    std::unique_ptr<GameFrameworkInternal::SessionState> myState;
    friend class GameApplication;
    friend class GameFrameworkIntegration::LegacySceneBridge;
};
