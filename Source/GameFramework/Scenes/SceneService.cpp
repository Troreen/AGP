#include "SceneService.h"
#include "../Runtime/Internal/SceneServiceAccess.h"
#include "../Runtime/Internal/WorldAccess.h"
#include <stdexcept>
SceneService::SceneService() : myState(std::make_unique<GameFrameworkInternal::SceneServiceState>()) {}
SceneService::~SceneService() = default;
bool SceneService::Load(SceneId id)
{
    if (myState->BoundWorld) GameFrameworkInternal::WorldAccess::EnsureMutationAllowed(*myState->BoundWorld);
    if (id.Value.empty()) throw std::invalid_argument("SceneId must not be empty");
    std::scoped_lock lock(myState->Mutex);
    if (!myState->Accepting) return false;
    myState->Pending = std::move(id);
    myState->Status = SceneLoadStatus::Requested;
    myState->Requested = true;
    return true;
}
bool SceneService::Reload()
{
    if (myState->BoundWorld) GameFrameworkInternal::WorldAccess::EnsureMutationAllowed(*myState->BoundWorld);
    auto current = GetCurrent();
    return current && Load(std::move(*current));
}
SceneLoadStatus SceneService::GetStatus() const { std::scoped_lock lock(myState->Mutex); return myState->Status; }
std::optional<SceneId> SceneService::GetCurrent() const { std::scoped_lock lock(myState->Mutex); return myState->Current; }
std::optional<SceneLoadError> SceneService::GetLastError() const { std::scoped_lock lock(myState->Mutex); return myState->Error; }
