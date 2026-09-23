#include "ServiceLocator.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AudioManager.h"
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

AudioManager* ServiceLocator::SetAudioManager(AudioManager* anAudioManager)
{
	if (myOwnedAudioManager != anAudioManager)
	{
		delete myOwnedAudioManager;
		myOwnedAudioManager = anAudioManager;
	}
	return anAudioManager;
}

AssetRegistry* ServiceLocator::SetAssetRegistry(AssetRegistry* anAssetRegistry)
{
	if (myOwnedAssetRegistry != anAssetRegistry)
	{
		delete myOwnedAssetRegistry;
		myOwnedAssetRegistry = anAssetRegistry;
	}
	return anAssetRegistry;
}

void ServiceLocator::KillServices()
{
	delete myOwnedInputMapper;
	myOwnedInputMapper = nullptr;
	delete myOwnedAudioManager;
	myOwnedAudioManager = nullptr;
	delete myOwnedAssetRegistry;
	myOwnedAssetRegistry = nullptr;
}

ServiceLocator::ServiceLocator()
	: myOwnedInputMapper(nullptr)
	, myOwnedAudioManager(nullptr)
	, myOwnedAssetRegistry(nullptr)
{}

ServiceLocator::~ServiceLocator()
{
	KillServices();
}

AudioManager& ServiceLocator::GetAudioManager() const
{
	if (!myOwnedAudioManager) throw std::logic_error("AudioManager service is unavailable");
	return *myOwnedAudioManager;
}

AssetRegistry& ServiceLocator::GetAssetRegistry() const
{
	if (!myOwnedAssetRegistry) throw std::logic_error("AssetRegistry service is unavailable");
	return *myOwnedAssetRegistry;
}
