#pragma once

namespace CommonUtilities
{
	class InputMapper;
}

class AudioManager;
class AssetRegistry;
class AnimationManager;
class EngineSettings;

class ServiceLocator
{
public:
	ServiceLocator(const ServiceLocator&) = delete;
	ServiceLocator& operator=(const ServiceLocator&) = delete;
	ServiceLocator(ServiceLocator&&) = delete;
	ServiceLocator& operator=(ServiceLocator&&) = delete;

	static ServiceLocator& GetInstance();

	CommonUtilities::InputMapper* GetInputMapper();
	AudioManager& GetAudioManager() const;
	AssetRegistry& GetAssetRegistry() const;
	EngineSettings& GetEngineSettings() const;
	AnimationManager& GetAnimationManager() const;

	// Every setter transfers ownership. Replace the mapper only after removing all its listeners.
	CommonUtilities::InputMapper* SetInputMapper(CommonUtilities::InputMapper* anInputMapper);
	AudioManager* SetAudioManager(AudioManager* anAudioManager);
	AssetRegistry* SetAssetRegistry(AssetRegistry* anAssetRegistry);
	EngineSettings* SetEngineSettings(EngineSettings* anEngineSettings);
	AnimationManager* SetAnimationManager(AnimationManager* anAnimationManager);
	void KillServices();

private:
	ServiceLocator();
	~ServiceLocator();

	CommonUtilities::InputMapper* myOwnedInputMapper;
	AudioManager* myOwnedAudioManager;
	AssetRegistry* myOwnedAssetRegistry;
	EngineSettings* myOwnedEngineSettings;
	AnimationManager* myAnimationManager;
};
