#pragma once

#include "GameFramework/Components/Component.h"

#include "Transform.hpp"
#include "TransformOperations.h"
#include "Vector3.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class World;

// A world-owned entity: transform, activation state and an owned set of components.
// Compose behavior with AddComponent rather than subclassing Actor (its destructor
// is not virtual). Persistent references use handles; raw pointers are temporary borrows.
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
    CommonUtilities::Transform& GetLocalTransform() { return myTransform; }
    const CommonUtilities::Transform& GetLocalTransform() const { return myTransform; }
    // World edits reject singular parents or local shear without changing the pose.
    bool SetWorldMatrix(const CommonUtilities::Matrix4f& matrix) { return GameFrameworkInternal::SetWorldMatrix(myTransform,matrix); }
    CommonUtilities::Matrix4f GetWorldMatrix() const { return myTransform.GetWorldMatrix(); }
    Actor* GetParent() const { return myParent.Get(); }
    bool IsLocallyActive() const { return myIsActive; }
    bool SetParent(Actor* parent, ReparentMode mode);

    // Constructors run before owner/name attachment. Configure the returned object;
    // Connect and BeginPlay run later at the world boundary, never inside AddComponent.
	template <typename T, typename... Args>
	T* AddComponent(std::string aName, Args&&... someArgs)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

		if (!CanAttach()) return nullptr;
		if (!CanAddComponentName(aName))
		{
			ReportDuplicateComponentName(aName);
			return nullptr;
		}

		auto component = std::make_unique<T>(std::forward<Args>(someArgs)...);
		T* rawComponent = component.get();
		rawComponent->SetOwner(this);
		rawComponent->SetName(std::move(aName));

		AttachComponent(std::move(component));
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
			if (T* casted = component->IsPendingDestroy() ? nullptr : dynamic_cast<T*>(component.get()))
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
			if (T* casted = component->IsPendingDestroy() ? nullptr : dynamic_cast<T*>(component.get()))
			{
				outComponents.push_back(casted);
			}
		}
	}

    template <typename T> bool RemoveComponent()
    {
        if (auto* component = GetComponent<T>()) { component->Destroy(); return true; }
        return false;
    }
    void Destroy();
    bool IsPendingDestroy() const { return myPendingDestroy; }
    ActorHandle GetHandle() const { return ActorHandle(myHandle); }
    const std::vector<std::unique_ptr<Component>>& GetComponents() const { return myComponents; }

	void RemoveAllComponents();

private:
	void SetWorld(World* aWorld);
    void AttachComponent(std::unique_ptr<Component> component);
    bool CanAttach() const;
    bool ApplyParent(Actor* parent, ReparentMode mode);
    ActorHandle myParent;
	bool CanAddComponentName(const std::string& aName) const;
	void ReportDuplicateComponentName(const std::string& aName) const;

	std::string myName;
	bool myIsActive = true;
    bool myPendingDestroy = false;
    ObjectHandle myHandle;
	World* myWorld = nullptr;
	CommonUtilities::Transform myTransform;
	std::vector<std::unique_ptr<Component>> myComponents;
    std::vector<std::unique_ptr<Component>> myPendingComponents;

	friend class World;
};
