#include "ServiceLocator.h"
#include <InputMapper.h>
#include <stdexcept>

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
	if (myOwnedInputMapper != anInputMapper)
	{
		delete myOwnedInputMapper;
		myOwnedInputMapper = anInputMapper;
	}
	return anInputMapper;
}

void ServiceLocator::KillServices()
{
	delete myOwnedInputMapper;
	myOwnedInputMapper = nullptr;
	myAudioManager = nullptr;
	myAssetRegistry = nullptr;
}

ServiceLocator::ServiceLocator() : myOwnedInputMapper(nullptr) {}

ServiceLocator::~ServiceLocator()
{
	KillServices();
}

AudioManager& ServiceLocator::GetAudioManager() const
{
	if (!myAudioManager) throw std::logic_error("AudioManager service is unavailable");
	return *myAudioManager;
}

AssetRegistry& ServiceLocator::GetAssetRegistry() const
{
	if (!myAssetRegistry) throw std::logic_error("AssetRegistry service is unavailable");
	return *myAssetRegistry;
}
