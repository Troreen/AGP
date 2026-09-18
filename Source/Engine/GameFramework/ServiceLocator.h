#pragma once

class AssetRegistry;
class AudioManager;
class InputSystem;

// Transitional access point for engine-wide services. Services are owned by the
// application/runtime; the locator only exposes their current non-owning references.
class ServiceLocator
{
public:
	static ServiceLocator& GetInstance();

	void ProvideInput(InputSystem& input) { myInput = &input; }
	void ProvideAudio(AudioManager& audio) { myAudio = &audio; }
	void ProvideAssets(AssetRegistry& assets) { myAssets = &assets; }

	InputSystem& GetInputSystem() const;
	AudioManager& GetAudioManager() const;
	AssetRegistry& GetAssetRegistry() const;
	void Clear();

private:
	InputSystem* myInput = nullptr;
	AudioManager* myAudio = nullptr;
	AssetRegistry* myAssets = nullptr;
};
