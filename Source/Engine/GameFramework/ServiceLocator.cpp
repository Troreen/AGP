#include "ServiceLocator.h"

#include "GameFramework/AudioManager.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/Animation/AnimationManager.h"
#include "GameFramework/Settings/EngineSettings.h"
#include "GameFramework/Coroutine/Scheduler.h"
#include "GameFramework/Coroutine/CoroutineHandler.h"

#include <InputMapper.h>

ServiceLocator& ServiceLocator::GetInstance()
{
	static ServiceLocator instance;
	return instance;
}

void ServiceLocator::Initialize()
{
	myInputMapper = new CommonUtilities::InputMapper();
	myAudioManager = new AudioManager();
	myAssetRegistry = new AssetRegistry();
	myAnimationManager = new AnimationManager();
	myEngineSettings = new EngineSettings();
	myScheduler = new Scheduler();
	myCouroutineHandler = new CoroutineHandler();
}

CommonUtilities::InputMapper& ServiceLocator::GetInputMapper()
{
	return *myInputMapper;
}

AudioManager& ServiceLocator::GetAudioManager() const
{
	return *myAudioManager;
}

AssetRegistry& ServiceLocator::GetAssetRegistry() const
{
	return *myAssetRegistry;
}

AnimationManager& ServiceLocator::GetAnimationManager() const
{
	return *myAnimationManager;
}

EngineSettings& ServiceLocator::GetEngineSettings() const
{
	return *myEngineSettings;
}

Scheduler& ServiceLocator::GetScheduler() const
{
	return *myScheduler;
}

CoroutineHandler& ServiceLocator::GetCoroutineHandler() const
{
	return *myCouroutineHandler;
}

void ServiceLocator::KillServices()
{
	delete myInputMapper;
	myInputMapper = nullptr;

	delete myAudioManager;
	myAudioManager = nullptr;

	delete myAssetRegistry;
	myAssetRegistry = nullptr;

	delete myAnimationManager;
	myAnimationManager = nullptr;

	// Settings stay available until the services that use them are gone.
	delete myEngineSettings;
	myEngineSettings = nullptr;

	delete myScheduler;
	myScheduler = nullptr;
}

ServiceLocator::ServiceLocator()
	: myInputMapper(nullptr)
	, myAudioManager(nullptr)
	, myAssetRegistry(nullptr)
	, myAnimationManager(nullptr)
	, myEngineSettings(nullptr)
	, myScheduler(nullptr)
	, myCouroutineHandler(nullptr)
{
}

ServiceLocator::~ServiceLocator()
{
	KillServices();
}
