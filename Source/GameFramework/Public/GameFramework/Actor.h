#pragma once
#include "GameFramework/Component.h"
#include "GameFramework/Transform.h"
#include "GameFramework/ReparentMode.h"
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class World;

namespace GameFrameworkInternal
{
	class WorldAccess;
}

// A world-owned component container. Display names can repeat; refs are identity.
// Returned pointers are short borrows. Additions start automatically next frame.
class Actor final
{
public:
	~Actor();
	Actor(const Actor&) = delete;
	Actor& operator=(const Actor&) = delete;
	Actor(Actor&&) = delete;
	Actor& operator=(Actor&&) = delete;
	const std::string& GetName() const;
	void SetName(std::string name);
	bool IsActive() const;

	bool IsActiveInHierarchy() const
	{
		return IsActive();
	}

	bool IsLocallyActive() const
	{
		return myIsActive;
	}

	void SetActive(bool active);

	Transform& GetTransform()
	{
		return myTransform;
	}

	const Transform& GetTransform() const
	{
		return myTransform;
	}

	World* GetWorld() const;

	Actor* GetParent() const
	{
		return myParent.Get();
	}

	// Immediate: false preserves both links and local pose.
	bool SetParent(Actor* parent, ReparentMode mode);

	template <class T> T* AddComponent()
	{
		return AddComponent<T>(NextComponentName());
	}

	template <class T, class... Args> T* AddComponent(std::string name, Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>);
		if (!CanAttach())
		{
			return nullptr;
		}
		if (!CanAddComponentName(name))
		{
			ReportDuplicateComponentName(name);
		}
		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		auto* result = component.get();
		result->SetOwner(this);
		result->SetName(std::move(name));
		AttachComponent(std::move(component));
		return result;
	}

	Component* FindComponent(const std::string& name) const;

	template <class T> T* FindComponent(const std::string& name) const
	{
		return dynamic_cast<T*>(FindComponent(name));
	}

	template <class T> T* GetComponent() const
	{
		for (const auto* list : {&myComponents, &myPendingComponents})
		{
			for (const auto& component : *list)
			{
				if (!component->IsPendingDestroy())
				{
					if (auto* result = dynamic_cast<T*>(component.get()))
					{
						return result;
					}
				}
			}
		}
		return nullptr;
	}

	template <class T> std::vector<T*> GetComponents() const
	{
		std::vector<T*> result;
		GetComponentsOfType(result);
		return result;
	}

	template <class T> void GetComponentsOfType(std::vector<T*>& result) const
	{
		for (const auto* list : {&myComponents, &myPendingComponents})
		{
			for (const auto& component : *list)
			{
				if (!component->IsPendingDestroy())
				{
					if (auto* typed = dynamic_cast<T*>(component.get()))
					{
						result.push_back(typed);
					}
				}
			}
		}
	}

	template <class T> bool RemoveComponent()
	{
		if (auto* c = GetComponent<T>())
		{
			c->Destroy();
			return true;
		}
		return false;
	}

	void RemoveAllComponents();
	void Destroy();

	bool IsPendingDestroy() const
	{
		return myPendingDestroy;
	}

	ActorRef GetRef() const
	{
		return ActorRef(myHandle);
	}

private:
	explicit Actor(std::string name);
	void FixedUpdate(float delta);
	void Update(float delta);
	void LateUpdate(float delta);

	const std::vector<std::unique_ptr<Component>>& GetComponents() const
	{
		return myComponents;
	}

	void SetWorld(World* world);
	void AttachComponent(std::unique_ptr<Component> component);
	bool CanAttach() const;
	bool CanAddComponentName(const std::string& name) const;
	void ReportDuplicateComponentName(const std::string& name) const;
	std::string NextComponentName() const;
	ActorRef myParent;
	std::string myName;
	bool myIsActive = true;
	bool myPendingDestroy = false;
	bool myAdmitted = false;
	GameFrameworkInternal::ObjectIdentity myHandle;
	World* myWorld = nullptr;
	Transform myTransform;
	std::vector<std::unique_ptr<Component>> myComponents;
	std::vector<std::unique_ptr<Component>> myPendingComponents;
	friend class World;
	friend class SceneComponent;
	friend class GameFrameworkInternal::WorldAccess;
};
