#include "ControllerFactory.h"
#include "FollowPathController.h"


template <typename T>
void ControllerFactory::Register(const std::string& aName)
{
	std::shared_ptr<T> t = std::make_shared<T>();
	myControllers.emplace(aName, std::move(t));
}

ControllerFactory::ControllerFactory()
{
	Register<FollowPathController>("Follow Path");
}

std::shared_ptr<Controller> ControllerFactory::GetController(const std::string aName)
{
	if (myControllers.contains(aName))
	{
		return myControllers[aName];
	}

	std::cout << "\nNo controller by the name " << aName << " exists!\n";
	return nullptr;
}
