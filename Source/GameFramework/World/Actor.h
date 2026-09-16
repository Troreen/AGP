#pragma once

#include "GameFramework/Components/Component.h"

#include "Transform.hpp"
#include "Vector3.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class World;

// A world-owned entity: transform, activation state and an owned set of components.
// Compose behavior with AddComponent rather than subclassing Actor (its destructor
// is not virtual). Components share this actor transform; there is no hierarchy here.
class Actor
{
public:
	explicit Actor(std::string aName);
	~Actor();

	Actor(const Actor&) = delete;
	Actor& operator=(const Actor&) = delete;
	Actor(Actor&&) = delete;
	Actor& operator=(Actor&&) = delete;

	// Called by World, with inactive actors skipped in every phase. Each enabled
	// component runs in attachment order, so order dependencies must be deliberate.
	void FixedUpdate(float aDeltaTime);
	void Update(float aDeltaTime);
	void LateUpdate(float aDeltaTime);

	const std::string& GetName() const;
	void SetName(std::string aName);

	bool IsActive() const;
	void SetActive(bool anIsActive);

	CommonUtilities::Transform& GetTransform();
	const CommonUtilities::Transform& GetTransform() const;

	void SetTranslation(const CommonUtilities::Vector3<float>& aTranslation);
	void SetPosition(const CommonUtilities::Vector3<float>& aPosition);
	void SetRotation(const CommonUtilities::Quaternion<float>& aRotation);
	void SetRotation(float aYawDegrees, float aPitchDegrees, float aRollDegrees);
	void SetScale(const CommonUtilities::Vector3<float>& aScale);
	void LookAt(const CommonUtilities::Vector3<float>& aTarget);

	World* GetWorld() const;

	// Construct and attach a component. Its constructor runs BEFORE owner/name are
	// assigned, so constructors must not use GetOwner(). There is no BeginPlay hook yet;
	// owner-dependent setup can happen explicitly after attachment or on the first tick.
	// The returned pointer is borrowed and becomes invalid when the component is removed.
	template <typename T, typename... Args>
	T* AddComponent(std::string aName, Args&&... someArgs)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

		if (!CanAddComponentName(aName))
		{
			ReportDuplicateComponentName(aName);
			return nullptr;
		}

		auto component = std::make_unique<T>(std::forward<Args>(someArgs)...);
		T* rawComponent = component.get();
		rawComponent->SetOwner(this);
		rawComponent->SetName(std::move(aName));

		myComponents.push_back(std::move(component));
		return rawComponent;
	}

	Component* FindComponent(const std::string& aName) const;

	template <typename T>
	T* FindComponent(const std::string& aName) const
	{
		static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
		return dynamic_cast<T*>(FindComponent(aName));
	}

	template <typename T>
	T* GetComponent() const
	{
		static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

		for (const std::unique_ptr<Component>& component : myComponents)
		{
			if (T* casted = dynamic_cast<T*>(component.get()))
			{
				return casted;
			}
		}

		return nullptr;
	}

	template <typename T>
	void GetComponentsOfType(std::vector<T*>& outComponents) const
	{
		for (const std::unique_ptr<Component>& component : myComponents)
		{
			if (T* casted = dynamic_cast<T*>(component.get()))
			{
				outComponents.push_back(casted);
			}
		}
	}

	// Immediate removal, not a deferred command. Do not remove components from inside
	// a tick over this actor, and do not retain references to a removed component.
	template <typename T>
	bool RemoveComponent()
	{
		static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

		for (auto it = myComponents.begin(); it != myComponents.end(); ++it)
		{
			if (dynamic_cast<T*>(it->get()))
			{
				(*it)->OnDestroy();
				myComponents.erase(it);
				return true;
			}
		}

		return false;
	}

	void RemoveAllComponents();

private:
	void SetWorld(World* aWorld);
	bool CanAddComponentName(const std::string& aName) const;
	void ReportDuplicateComponentName(const std::string& aName) const;

	std::string myName;
	bool myIsActive = true;
	World* myWorld = nullptr;
	CommonUtilities::Transform myTransform;
	std::vector<std::unique_ptr<Component>> myComponents;

	friend class World;
};
