#pragma once
#include "GameFramework/World/Component.h"
#include "GameFramework/World/Transform.h"
#include <memory>
#include <vector>
#include <utility>
#include <type_traits>
#include <algorithm>

class World;
class WorldRenderer;

// The World owns Actors; Actors own Components. Pointers are borrowed, not handles.
// Callers must not retain them across actor/component destruction or scene replacement.
class Actor final
{
public:
	~Actor();
	Actor(const Actor&) = delete;
	Actor& operator=(const Actor&) = delete;

	const std::string& GetName() const { return myName; }
	const std::string& GetArchetype() const { return myArchetype; }
	const std::vector<std::string>& GetTags() const { return myTags; }
	bool HasTag(const std::string& tag) const { return std::find(myTags.begin(), myTags.end(), tag) != myTags.end(); }

	World* GetWorld() const	{ return myWorld; }

	Transform& GetTransform() { return myTransform; }

	const Transform& GetTransform() const{ return myTransform; }

	bool IsActive() const { return myActive && !myDestroyed; }

	void SetActive(bool active) { myActive = active; }

	bool IsPendingDestroy() const { return myDestroyed; }

	// Destruction is deferred so callbacks cannot invalidate World iteration.
	void Destroy()
	{
		myDestroyed = true;
	}

	template <class T, class... Args> T* AddComponent(std::string name, Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>);
		std::unique_ptr<T> component = std::make_unique<T>(std::forward<Args>(args)...);
		T* attachedComponent = component.get();
		Attach(std::move(component), std::move(name));
		return attachedComponent;
	}

	template <class T> T* AddComponent()
	{
		return AddComponent<T>("Component " + std::to_string(++myNextComponentName));
	}

	Component* FindComponent(const std::string& name) const;

	template <class T> T* GetComponent() const
	{
		for (const std::unique_ptr<Component>& component : myComponents)
		{
			if (component->IsPendingDestroy())
			{
				continue;
			}
			if (T* foundComponent = dynamic_cast<T*>(component.get()))
			{
				return foundComponent;
			}
		}
		return nullptr;
	}

private:
	Actor(World& world, std::string name);
	void Attach(std::unique_ptr<Component> component, std::string name);
	std::string myName;
	std::string myArchetype;
	std::vector<std::string> myTags;
	World* myWorld;
	Transform myTransform;
	std::vector<std::unique_ptr<Component>> myComponents;
	size_t myNextComponentName = 0;
	bool myActive = true;
	bool myDestroyed = false;
	friend class World;
	friend class WorldRenderer;
	friend class ComponentRegistry;
};
