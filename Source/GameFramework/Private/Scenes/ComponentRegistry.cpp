#include "GameFramework/Registration/ComponentRegistry.h"
#include "Runtime/Internal/WorldAccess.h"

Component* ComponentRegistry::Create(const std::string& type, Actor& actor, const std::string& name) const
{
	if (!myFrozen)
	{
		throw std::logic_error("Freeze registrations before scene construction");
	}
	const auto it = myTypes.find(type);
	if (it == myTypes.end())
	{
		throw std::invalid_argument("Unknown component type: " + type);
	}
	std::unique_ptr<Component> component;
	GameFrameworkInternal::WorldAccess::Construct([&component, it]
	{
		component = it->second.Factory();
	});
	if (!component)
	{
		throw std::runtime_error("Component factory returned no component");
	}
	return GameFrameworkInternal::WorldAccess::AttachComponent(actor, name, std::move(component));
}
