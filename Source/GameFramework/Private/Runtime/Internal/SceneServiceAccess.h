#pragma once
#include "GameFramework/SceneService.h"
#include <atomic>
#include <mutex>
class World;

namespace GameFrameworkInternal
{
	struct SceneServiceState
	{
		std::mutex Mutex;
		std::optional<SceneId> Pending;
		std::optional<SceneId> Current;
		std::optional<SceneLoadError> Error;
		SceneLoadStatus Status = SceneLoadStatus::Idle;
		std::atomic<bool> Requested = false;
		bool Accepting = true;
		World* BoundWorld = nullptr;
	};

	class SceneServiceAccess
	{
	public:
		static SceneServiceState& Get(SceneService& scenes)
		{
			return *scenes.myState;
		}
	};
}
