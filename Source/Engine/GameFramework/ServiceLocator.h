#pragma once

namespace CommonUtilities
{
	class InputMapper;
}

class AudioManager;
class EventBus;
class AssetRegistry;
class AnimationManager;
class EngineSettings;
class Scheduler;
class CoroutineHandler;
class CameraComponent;
class Actor;

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
	AudioManager& GetAudioManager() const;
	EventBus& GetEventBus() const;
	AssetRegistry& GetAssetRegistry() const;
	AnimationManager& GetAnimationManager() const;
	EngineSettings& GetEngineSettings() const;
	Scheduler& GetScheduler() const;
	CoroutineHandler& GetCoroutineHandler() const;

	CameraComponent& GetActiveCamera() const;
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
	AudioManager* myAudioManager;
	EventBus* myEventBus;
	AssetRegistry* myAssetRegistry;
	AnimationManager* myAnimationManager;
	EngineSettings* myEngineSettings;
	Scheduler* myScheduler;
	CoroutineHandler* myCouroutineHandler;
	CameraComponent* mySceneCamera;
	Actor* myPlayer;

	float myDeltaTime;
};
