#pragma once

#include <random>

namespace CommonUtilities
{
	class InputMapper;
}

class Pathfinder;
class AudioManager;
class EventBus;
class AssetRegistry;
class AnimationManager;
class EngineSettings;
class Scheduler;
class CoroutineHandler;
class CameraComponent;
class Actor;
class SceneManager;

class ServiceLocator
{
public:
	ServiceLocator(const ServiceLocator&) = delete;
	ServiceLocator& operator=(const ServiceLocator&) = delete;
	ServiceLocator(ServiceLocator&&) = delete;
	ServiceLocator& operator=(ServiceLocator&&) = delete;

	static ServiceLocator& GetInstance();

	void Initialize();

	CommonUtilities::InputMapper& GetInputMapper();
	Pathfinder& GetPathFinder() const;
	AudioManager& GetAudioManager() const;
	EventBus& GetEventBus() const;
	AssetRegistry& GetAssetRegistry() const;
	AnimationManager& GetAnimationManager() const;
	EngineSettings& GetEngineSettings() const;
	Scheduler& GetScheduler() const;
	CoroutineHandler& GetCoroutineHandler() const;
	SceneManager& GetSceneManager() const;
	std::mt19937& GetRngEngine();

	CameraComponent* GetActiveCamera() const;
	void SetActiveCamera(CameraComponent* aCamera);

	Actor* GetPlayer() const;
	void SetPlayer(Actor* aPlayer);

	float GetDeltaTime() const;
	void SetDeltaTime(float aDeltaTime);

	void KillServices();

private:
	ServiceLocator();
	~ServiceLocator();

	CommonUtilities::InputMapper* myInputMapper;
	Pathfinder* myPathFinder;
	AudioManager* myAudioManager;
	EventBus* myEventBus;
	AssetRegistry* myAssetRegistry;
	AnimationManager* myAnimationManager;
	EngineSettings* myEngineSettings;
	Scheduler* myScheduler;
	CoroutineHandler* myCouroutineHandler;
	CameraComponent* mySceneCamera;
	Actor* myPlayer;
	SceneManager* mySceneManager;
	std::mt19937 myRngEngine;

	float myDeltaTime;
};
