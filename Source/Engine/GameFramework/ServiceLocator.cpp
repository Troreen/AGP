#include "ServiceLocator.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AudioManager.h"
#include "GameFramework/Animation/AnimationManager.h"
#include "GameFramework/Settings/EngineSettings.h"
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

AnimationManager* ServiceLocator::SetAnimationManager(AnimationManager* anAnimationManager)
{
	if (myAnimationManager != anAnimationManager)
	{
		delete myAnimationManager;
		myAnimationManager = anAnimationManager;
	}
	return anAnimationManager;
}

EngineSettings* ServiceLocator::SetEngineSettings(EngineSettings* anEngineSettings)
{
	if (myOwnedEngineSettings != anEngineSettings)
	{
		delete myOwnedEngineSettings;
		myOwnedEngineSettings = anEngineSettings;
	}
	return anEngineSettings;
}


void ServiceLocator::KillServices()
{
	delete myOwnedInputMapper;
	myOwnedInputMapper = nullptr;
	delete myOwnedAudioManager;
	myOwnedAudioManager = nullptr;
	delete myOwnedAssetRegistry;
	myOwnedAssetRegistry = nullptr;
	delete myAnimationManager;
	myAnimationManager = nullptr;
	// Settings stay available until the services that use them are gone.
	delete myOwnedEngineSettings;
	myOwnedEngineSettings = nullptr;
}

ServiceLocator::ServiceLocator()
	: myOwnedInputMapper(nullptr)
	, myOwnedAudioManager(nullptr)
	, myOwnedAssetRegistry(nullptr)
	, myOwnedEngineSettings(nullptr)
	, myAnimationManager(nullptr)
{}

ServiceLocator::~ServiceLocator()
{
	KillServices();
}

AudioManager& ServiceLocator::GetAudioManager() const
{
	if (!myOwnedAudioManager)
	{
		throw std::logic_error("AudioManager service is unavailable");
	}

	return *myOwnedAudioManager;
}

AssetRegistry& ServiceLocator::GetAssetRegistry() const
{
	if (!myOwnedAssetRegistry)
	{
		throw std::logic_error("AssetRegistry service is unavailable");
	}

	return *myOwnedAssetRegistry;
}

AnimationManager& ServiceLocator::GetAnimationManager() const
{
	if (!myAnimationManager)
	{
		throw std::logic_error("AnimationManager service is unavailable");
	}
	return *myAnimationManager;
}

EngineSettings& ServiceLocator::GetEngineSettings() const
{
	if (!myOwnedEngineSettings)
	{
		throw std::logic_error("EngineSettings service is unavailable");
	}

	return *myOwnedEngineSettings;
}
