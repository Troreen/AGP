#pragma once

namespace CommonUtilities
{
	class InputMapper;
}

class AudioManager;
class AssetRegistry;

class ServiceLocator
{
	public:
		ServiceLocator(const ServiceLocator&) = delete;
		ServiceLocator& operator=(const ServiceLocator&) = delete;

		ServiceLocator(ServiceLocator&&) = delete;
		ServiceLocator& operator=(ServiceLocator&&) = delete;

		static ServiceLocator& GetInstance();

		CommonUtilities::InputMapper* GetInputMapper();
		// Every setter transfers ownership. Replacing a service deletes the old one.
		// Replace the mapper only after all old listeners have been removed.
		CommonUtilities::InputMapper* SetInputMapper(CommonUtilities::InputMapper* anInputMapper);
		AudioManager* SetAudioManager(AudioManager* anAudioManager);
		AssetRegistry* SetAssetRegistry(AssetRegistry* anAssetRegistry);
		AudioManager& GetAudioManager() const;
		AssetRegistry& GetAssetRegistry() const;

		void KillServices();

	private:
		ServiceLocator();
		~ServiceLocator();

		CommonUtilities::InputMapper* myOwnedInputMapper;
		AudioManager* myOwnedAudioManager;
		AssetRegistry* myOwnedAssetRegistry;
};
