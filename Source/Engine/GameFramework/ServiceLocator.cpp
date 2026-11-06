#include "GameFramework.pch.h"
#include "ServiceLocator.h"

#include "GameFramework/AudioManager.h"
#include "GameFramework/EventBus.hpp"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/Animation/AnimationManager.h"
#include "GameFramework/Settings/EngineSettings.h"
#include "GameFramework/Coroutine/Scheduler.h"
#include "GameFramework/Coroutine/CoroutineHandler.h"
#include <GameFramework/Components/CameraComponent.h>
#include "GameFramework/Scenes/SceneManager.h"
#include <GameFramework/World/Actor.h>
#include <AISystems/Pathfinder.hpp>

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
	myPathFinder = new Pathfinder();
	myEventBus = new EventBus();
	myAssetRegistry = new AssetRegistry();
	myAnimationManager = new AnimationManager();
	myEngineSettings = new EngineSettings();
	myScheduler = new Scheduler();
	myCouroutineHandler = new CoroutineHandler();
	mySceneManager = new SceneManager();
	std::random_device seed;
	myRngEngine = std::mt19937(seed());
}

CommonUtilities::InputMapper& ServiceLocator::GetInputMapper()
{
	return *myInputMapper;
}

Pathfinder& ServiceLocator::GetPathFinder() const
{
	return *myPathFinder;
}

AudioManager& ServiceLocator::GetAudioManager() const
{
	return *myAudioManager;
}

EventBus& ServiceLocator::GetEventBus() const
{
	return *myEventBus;
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

SceneManager& ServiceLocator::GetSceneManager() const
{
	return *mySceneManager;
}

std::mt19937& ServiceLocator::GetRngEngine()
{
	return myRngEngine;
}

CameraComponent* ServiceLocator::GetActiveCamera() const
{
	return mySceneCamera;
}

void ServiceLocator::SetActiveCamera(CameraComponent* aCamera)
{
	mySceneCamera = aCamera;
}

Actor* ServiceLocator::GetPlayer() const
{
	return myPlayer;
}

float ServiceLocator::GetDeltaTime() const
{
	return myDeltaTime;
}


void ServiceLocator::SetPlayer(Actor* aPlayer)
{
	myPlayer = aPlayer;
}

void ServiceLocator::SetDeltaTime(float aDeltaTime)
{
	myDeltaTime = aDeltaTime;
}

void ServiceLocator::KillServices()
{
	delete myInputMapper;
	myInputMapper = nullptr;

	delete myAudioManager;
	myAudioManager = nullptr;

	delete myPathFinder;
	myPathFinder = nullptr;

	delete myEventBus;
	myEventBus = nullptr;

	delete myAssetRegistry;
	myAssetRegistry = nullptr;

	delete myAnimationManager;
	myAnimationManager = nullptr;

	delete myScheduler;
	myScheduler = nullptr;

	delete mySceneManager;
	mySceneManager = nullptr;

	// Settings stay available until the services that use them are gone.
	delete myEngineSettings;
	myEngineSettings = nullptr;
}

ServiceLocator::ServiceLocator()
	: myInputMapper(nullptr)
	, myAudioManager(nullptr)
	, myPathFinder(nullptr)
	, myEventBus(nullptr)
	, myAssetRegistry(nullptr)
	, myAnimationManager(nullptr)
	, myEngineSettings(nullptr)
	, myScheduler(nullptr)
	, myCouroutineHandler(nullptr)
	, mySceneCamera(nullptr)
	, mySceneManager(nullptr)
	, myPlayer(nullptr)
	, myDeltaTime(0)
{
}

ServiceLocator::~ServiceLocator()
{
	KillServices();
}
