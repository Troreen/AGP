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
		// Transfers ownership; replace only after all old listeners have been removed.
		CommonUtilities::InputMapper* SetInputMapper(CommonUtilities::InputMapper* anInputMapper);

		// Audio owns its singleton lifetime; AssetRegistry has static lifetime.
		// These setters borrow services. KillServices only clears their pointers.
		void SetAudioManager(AudioManager* audio) { myAudioManager = audio; }
		void SetAssetRegistry(AssetRegistry* assets) { myAssetRegistry = assets; }
		AudioManager& GetAudioManager() const;
		AssetRegistry& GetAssetRegistry() const;

		void KillServices();

	private:
		ServiceLocator();
		~ServiceLocator();

		CommonUtilities::InputMapper* myOwnedInputMapper;
		AudioManager* myAudioManager = nullptr;
		AssetRegistry* myAssetRegistry = nullptr;
};
