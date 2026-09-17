#pragma once
#include "GameFramework/World/World.h"
#include "GameFramework/Scenes/SceneReader.h"
#include <unordered_map>

// Registered factories attach one component, then apply its properties.
class ComponentRegistry
{
public:
	template <class T> void Register(std::string type, std::function<void(T&, const SceneReader&)> configure = {})
	{
		if (type.empty() || myFactories.contains(type))
		{
			throw std::invalid_argument("Duplicate or empty component type: " + type);
		}
		myFactories.emplace(std::move(type), [configure](Actor& actor, const ComponentRecord& record, const SceneReader& fields)
		{
			auto* component = actor.AddComponent<T>(record.Name);
			if (configure)
			{
				configure(*component, fields);
			}
			return component;
		});
	}

	std::unique_ptr<World> CreateWorld(const SceneData& scene, const AssetLibrary& assets, const GameInput* input = nullptr,
	                                   CommonUtilities::Vector2u size = {1280, 720}) const;
	void RegisterBuiltIns();

private:
	using Factory = std::function<Component*(Actor&, const ComponentRecord&, const SceneReader&)>;
	std::unordered_map<std::string, Factory> myFactories;
};
