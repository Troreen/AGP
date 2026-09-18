#include "GameFramework/ServiceLocator.h"

#include <stdexcept>

ServiceLocator& ServiceLocator::GetInstance()
{
	static ServiceLocator instance;
	return instance;
}

InputSystem& ServiceLocator::GetInputSystem() const
{
	if (!myInput) throw std::logic_error("InputSystem service is unavailable");
	return *myInput;
}

AudioManager& ServiceLocator::GetAudioManager() const
{
	if (!myAudio) throw std::logic_error("AudioManager service is unavailable");
	return *myAudio;
}

AssetRegistry& ServiceLocator::GetAssetRegistry() const
{
	if (!myAssets) throw std::logic_error("AssetRegistry service is unavailable");
	return *myAssets;
}

void ServiceLocator::Clear()
{
	myInput = nullptr;
	myAudio = nullptr;
	myAssets = nullptr;
}
