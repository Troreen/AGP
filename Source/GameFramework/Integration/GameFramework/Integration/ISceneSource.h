#pragma once
#include "GameFramework/Integration/SceneData.h"
#include "GameFramework/Integration/AssetBindings.h"
#include "GameFramework/SceneService.h"
#include "Vector2.hpp"
#include <filesystem>
#include <optional>

namespace GameFrameworkIntegration
{
	struct SceneLoadContext
	{
		const std::filesystem::path& ContentRoot;
		CommonUtilities::Vector2u ClientSize;
		AssetBindings& Assets;
	};

	struct SceneSourceResult
	{
		std::optional<SceneData> Data;
		SceneDiagnostics Diagnostics;
	};

	// Invoked synchronously at the host's safe loading point. Return owned data,
	// never live objects. No Data means failure; an empty SceneData is valid.
	class ISceneSource
	{
	public:
		virtual ~ISceneSource() = default;
		virtual SceneSourceResult Load(const SceneId&, SceneLoadContext&) = 0;
	};

	struct ApplicationSetup
	{
		std::unique_ptr<ISceneSource> SceneSource;
	};
}
