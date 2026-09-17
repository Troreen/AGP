#include "AudioManager.h"

void AudioManager::Init()
{
	std::string bankRootPath = "../../../Dependencies/FMod/Desktop/";
	SoundEngine::Init(bankRootPath);

	SoundEngine::LoadBank("Master.strings.bank", 0);
	SoundEngine::LoadBank("Master.Bank", 0);

	SoundEngine::LoadBus("bus:/SFX", "SFX");
	myBusses.insert({ BusID::eSFX, "SFX" });
	SoundEngine::LoadBus("bus:/MUSIC", "MUSIC");
	myBusses.insert({ BusID::eMusic, "MUSIC" });

	myListener = SoundEngine::GetNextFreeListener();

	myMasterVolume = 1.f;
	myBusVolume[0] = 1.f;
	myBusVolume[1] = 1.f;

	SoundEngine::OverrideOtherListeners(myListener);

	RegisterAllEvents();
}

void AudioManager::Update(const float aDeltaTime)
{
	aDeltaTime;
	SoundEngine::Update();

	if (myFadeOutActive)
	{
		myMusicVolume -= myFadeSpeed;
		if (myMusicVolume <= 0.0f)
		{
			SetEventVolume(myFadeID, 0.0f);
			StopMusic(myFadeID, true);
			myFadeOutActive = false;

		}
		SetEventVolume(myFadeID, myMusicVolume);
	}
	else if (myFadeInActive)
	{
		myMusicVolume += myFadeSpeed;
		if (myMusicVolume > 1.0f)
		{
			SetEventVolume(myFadeID, 1.0f);
			PlayMusic(myFadeID, true);
			myFadeInActive = false;
		}
		SetEventVolume(myFadeID, myMusicVolume);
	}
}

void AudioManager::PlaySFX(const SoundID aSoundID)
{
	SoundEngine::PlayEventOneShot(aSoundID);
}

void AudioManager::PlayMusic(const SoundID aMusicID, const bool aUninterrupted)
{
	if (aUninterrupted)
	{
		if (!IsEventPlaying(aMusicID))
		{
			SoundEngine::PlayEvent(myMusicList[aMusicID]);
		}
	}
	else if (!aUninterrupted)
	{
		SoundEngine::PlayEvent(myMusicList[aMusicID]);
	}
}

void AudioManager::PlaySFXAtLocation(const SoundID aMusicID, const CommonUtilities::Vector3f aPosition)
{
	SoundEngine::PlayEventAtLocation(myMusicList[aMusicID], { aPosition.x * 0.001f, aPosition.y * 0.001f, 0.0f });
}

void AudioManager::StopMusic(const SoundID aMusicID, const bool anImmediately)
{
	SoundEngine::StopEvent(myMusicList[aMusicID], anImmediately);
}

void AudioManager::StopAllEvents()
{
	for (const auto& pair : myMusicList)
	{
		if (SoundEngine::IsEventPlaying(myMusicList[pair.first]))
		{
			SoundEngine::StopEvent(myMusicList[pair.first], true);
		}
	}
}

const bool AudioManager::IsEventPlaying(const SoundID aMusicID)
{
	return SoundEngine::IsEventPlaying(myMusicList[aMusicID]);
}

void AudioManager::SetBusVolume(BusID anID, float aPercentage)
{
	SoundEngine::SetBusVolume(myBusses[anID], aPercentage);
}

void AudioManager::SetMasterVolume(float aPercentage)
{
	SoundEngine::SetMasterVolume(aPercentage);
}

void AudioManager::SetEventVolume(const SoundID aSoundID, float aPercentage)
{
	SoundEngine::SetEventVolume(myMusicList[aSoundID], aPercentage);
}

void AudioManager::SetAudioListener(const CommonUtilities::Vector3f aPosition)
{
	myListenerPosition = aPosition;
	SoundEngine::SetListenerPosition(myListener, { myListenerPosition.x * 0.001f, myListenerPosition.y * 0.001f, 0.0f });
}

const CommonUtilities::Vector3f& AudioManager::GetAudioListenerPosition() const
{
	return myListenerPosition;
}

void AudioManager::FadeOut(const SoundID anID, const float aFadeSpeed)
{
	myFadeID = anID;
	myFadeSpeed = aFadeSpeed;
	myFadeOutActive = true;
	myMusicVolume = 1.0f;
}

void AudioManager::FadeIn(const SoundID anID, const float aFadeSpeed)
{
	myFadeID = anID;
	myFadeSpeed = aFadeSpeed;
	myFadeInActive = true;
	myMusicVolume = 0.0f;
}

void AudioManager::ResetSounds()
{
	myFadeOutActive = false;
	myFadeInActive = false;
}

void AudioManager::RegisterAllEvents()
{
	SoundEngine::RegisterEvent("event:/FMODTest", SoundID::eMainTheme);
	myMusicList.insert({ SoundID::eMainTheme, SoundEngine::CreateEventInstance(SoundID::eMainTheme) });
}

AudioManager::AudioManager()
{
	myMusicVolume = 1;
	myEventVolume = 1;
}

AudioManager* AudioManager::GetInstance()
{
	if (myInstance == nullptr)
	{
		myInstance = new AudioManager();
	}
	return myInstance;
}

void AudioManager::Shutdown()
{
	myInstance->myMusicList.clear();
	myInstance->myBusses.clear();
	SoundEngine::Release();
	delete myInstance;
	myInstance = nullptr;
}