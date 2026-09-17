#include "ServiceLocator.h"
#include <InputMapper.h>

ServiceLocator& ServiceLocator::GetInstance()
{
	static ServiceLocator instance;
	return instance;
}

CommonUtilities::InputMapper* ServiceLocator::GetInputMapper()
{
	return myOwnedInputMapper;
}

CommonUtilities::InputMapper* ServiceLocator::SetInputMapper(CommonUtilities::InputMapper* anInputMapper)
{
	myOwnedInputMapper = anInputMapper;
	return anInputMapper;
}

void ServiceLocator::KillServices()
{
	delete myOwnedInputMapper;
	myOwnedInputMapper = nullptr;
}

ServiceLocator::ServiceLocator() : myOwnedInputMapper(nullptr) {}

ServiceLocator::~ServiceLocator()
{
	KillServices();
}
