#pragma once
#include "GameFramework/World/Component.h"
#include "GameFramework/World/Transform.h"
#include <memory>
#include <vector>
#include <utility>
#include <type_traits>

class World;
class WorldRenderer;

// The World owns Actors; Actors own Components. Pointers are borrowed, not handles.
class Actor final
{
public:
	~Actor();
	Actor(const Actor&) = delete;
	Actor& operator=(const Actor&) = delete;

	const std::string& GetName() const
	{
		return myName;
	}

	World* GetWorld() const
	{
		return myWorld;
	}

	Transform& GetTransform()
	{
		return myTransform;
	}

	const Transform& GetTransform() const
	{
		return myTransform;
	}

	bool IsActive() const
	{
		return myActive && !myDestroyed;
	}

	void SetActive(bool active)
	{
		myActive = active;
	}

	bool IsPendingDestroy() const
	{
		return myDestroyed;
	}

	void Destroy()
	{
		myDestroyed = true;
	}

	template <class T, class... Args> T* AddComponent(std::string name, Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>);
		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		auto* result = component.get();
		Attach(std::move(component), std::move(name));
		return result;
	}

	template <class T> T* AddComponent()
	{
		return AddComponent<T>("Component " + std::to_string(++myNextComponentName));
	}

	Component* FindComponent(const std::string& name) const;

	template <class T> T* GetComponent() const
	{
		for (const auto& component : myComponents)
		{
			if (!component->IsPendingDestroy())
			{
				if (auto* found = dynamic_cast<T*>(component.get()))
				{
					return found;
				}
			}
		}
		return nullptr;
	}

private:
	Actor(World& world, std::string name);
	void Attach(std::unique_ptr<Component> component, std::string name);
	std::string myName;
	World* myWorld;
	Transform myTransform;
	std::vector<std::unique_ptr<Component>> myComponents;
	size_t myNextComponentName = 0;
	bool myActive = true;
	bool myDestroyed = false;
	friend class World;
	friend class WorldRenderer;
};
