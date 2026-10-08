#pragma once

namespace CommonUtilities
{
	class InputMapper;
}

class AudioManager;
class AssetRegistry;
class AnimationManager;
class EngineSettings;
class Scheduler;
class CoroutineHandler;

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
	AssetRegistry& GetAssetRegistry() const;
	AnimationManager& GetAnimationManager() const;
	EngineSettings& GetEngineSettings() const;
	Scheduler& GetScheduler() const;
	CoroutineHandler& GetCoroutineHandler() const;

	void KillServices();

private:
	ServiceLocator();
	~ServiceLocator();

	CommonUtilities::InputMapper* myInputMapper;
	AudioManager* myAudioManager;
	AssetRegistry* myAssetRegistry;
	AnimationManager* myAnimationManager;
	EngineSettings* myEngineSettings;
	Scheduler* myScheduler;
	CoroutineHandler* myCouroutineHandler;
};
