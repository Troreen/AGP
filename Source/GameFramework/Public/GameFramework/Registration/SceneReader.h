#pragma once
#include "GameFramework/AssetRefs.h"
#include "GameFramework/ObjectRef.h"
#include "GameFramework/SceneDiagnostic.h"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include "Quaternion.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Actor;
class Component;
class SceneBuilder;
struct ComponentRecord;
struct ObjectAddress;

enum class ReferenceRequirement
{
	Required,
	Optional
};

// Registration readers configure only their own component. Present values must
// have the requested type; every supplied field must be consumed. Bind targets
// must be Ref fields on the component, never stack temporaries.
class SceneReader
{
public:
	~SceneReader();
	SceneReader(const SceneReader&) = delete;
	SceneReader& operator=(const SceneReader&) = delete;
	bool Has(const std::string& field) const;
	float OptionalFloat(const std::string& field, float fallback);
	bool OptionalBool(const std::string& field, bool fallback);
	std::string OptionalString(const std::string& field, std::string fallback = {});
	CommonUtilities::Vector3f OptionalVector3(const std::string& field, CommonUtilities::Vector3f fallback = {});
	CommonUtilities::Quaternion<float> OptionalQuaternion(const std::string& field, CommonUtilities::Quaternion<float> fallback = {});
	CommonUtilities::Vector4f OptionalColor(const std::string& field, CommonUtilities::Vector4f fallback = {});
	AssetId OptionalAsset(const std::string& field, AssetId fallback = {});
	std::vector<AssetId> OptionalAssets(const std::string& field, std::vector<AssetId> fallback = {});
	AssetId RequiredAsset(const std::string& field);
	void Error(std::string field, std::string message);
	const AssetLookup& GetAssets() const;
	CommonUtilities::Vector2u GetClientSize() const;
	void BindActor(const std::string& field, ActorRef& target, ReferenceRequirement requirement);

	template <class T> void BindComponent(const std::string& field, ComponentRef<T>& target, ReferenceRequirement requirement)
	{
		Bind(field, true, requirement, [&target](Actor*, Component* component)
		{
			auto* typed = dynamic_cast<T*>(component);
			if (!typed)
			{
				return false;
			}
			target = typed->template GetRef<T>();
			return true;
		});
	}

private:
	class Impl;
	std::unique_ptr<Impl> myImpl;
	SceneReader(const ComponentRecord& record, SceneDiagnostic source, SceneDiagnostics& diagnostics, const AssetLookup* assets,
	            CommonUtilities::Vector2u clientSize, std::function<Actor*(const std::string&)> actorLookup,
	            std::function<Component*(const ObjectAddress&)> componentLookup);
	void Bind(const std::string& field, bool component, ReferenceRequirement requirement, std::function<bool(Actor*, Component*)> assign);
	void Finish();
	void Resolve();
	friend class SceneBuilder;
};
