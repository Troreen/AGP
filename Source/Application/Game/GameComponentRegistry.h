#pragma once
#include "GameFramework/Scenes/SceneData.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/Components/CustomComponent.h"
#include <functional>
#include <stdexcept>
#include <unordered_map>

class GameComponentRegistry
{
public:
	using Creator = std::function<CustomComponent*(Actor&, const CustomComponentData&)>;

	template <class T> void Register(const std::string& aCustomType)
	{
		static_assert(std::is_base_of_v<CustomComponent, T>);
		Creator aCreator = [](Actor& anActor, const CustomComponentData& aCustomComponentData) -> CustomComponent*
		{
			return anActor.AddComponent<T>(aCustomComponentData.Common.Name);
		};
		const bool registered = myCreators.emplace(aCustomType, std::move(aCreator)).second;
		if (!registered)
		{
			throw std::invalid_argument("Duplicate custom component registration: " + aCustomType);
		}
	}

	CustomComponent* Create(Actor& anActor, const CustomComponentData& aCustomComponentData) const
	{
		std::string customTypeName = aCustomComponentData.CustomType;
		if (customTypeName.ends_with("_C"))
		{
			customTypeName.resize(customTypeName.size() - 2);
		}
		const auto creator = myCreators.find(customTypeName);
		if (creator == myCreators.end())
		{
			return nullptr;
		}
		return creator->second(anActor, aCustomComponentData);
	}

private:
	std::unordered_map<std::string, Creator> myCreators;
};
