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
	myInitialized = true;
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
	SoundEngine::RegisterEvent("event:/Blizzard Ambience", SoundID::eBlizzardAmbience);
	SoundEngine::RegisterEvent("event:/Castle Ambience 1", SoundID::eCastleAmbienceOne);
	SoundEngine::RegisterEvent("event:/Castle Ambience 2", SoundID::eCastleAmbienceTwo);
	SoundEngine::RegisterEvent("event:/Collect Item", SoundID::eCollectItem);
	SoundEngine::RegisterEvent("event:/Crossbow Cock", SoundID::eCrossbowCock);
	SoundEngine::RegisterEvent("event:/Heavy Roar", SoundID::eHeavyRoar);
	SoundEngine::RegisterEvent("event:/Intro Swell", SoundID::eIntroSwell);
	SoundEngine::RegisterEvent("event:/Potion Drink", SoundID::ePotion);
	SoundEngine::RegisterEvent("event:/Sickle Slash", SoundID::eSickleSlash);
	SoundEngine::RegisterEvent("event:/Slash Hit", SoundID::eSlashHit);
	SoundEngine::RegisterEvent("event:/Whirlwind", SoundID::eWhirlwind);
	myMusicList.insert({ SoundID::eMainTheme, SoundEngine::CreateEventInstance(SoundID::eMainTheme) });
	myMusicList.insert({ SoundID::eBlizzardAmbience, SoundEngine::CreateEventInstance(SoundID::eBlizzardAmbience) });
	myMusicList.insert({ SoundID::eCastleAmbienceOne, SoundEngine::CreateEventInstance(SoundID::eCastleAmbienceOne) });
	myMusicList.insert({ SoundID::eCastleAmbienceTwo, SoundEngine::CreateEventInstance(SoundID::eCastleAmbienceTwo) });
	myMusicList.insert({ SoundID::eCollectItem, SoundEngine::CreateEventInstance(SoundID::eCollectItem) });
	myMusicList.insert({ SoundID::eCrossbowCock, SoundEngine::CreateEventInstance(SoundID::eCrossbowCock) });
	myMusicList.insert({ SoundID::eHeavyRoar, SoundEngine::CreateEventInstance(SoundID::eHeavyRoar) });
	myMusicList.insert({ SoundID::eIntroSwell, SoundEngine::CreateEventInstance(SoundID::eIntroSwell) });
	myMusicList.insert({ SoundID::ePotion, SoundEngine::CreateEventInstance(SoundID::ePotion) });
	myMusicList.insert({ SoundID::eSickleSlash, SoundEngine::CreateEventInstance(SoundID::eSickleSlash) });
	myMusicList.insert({ SoundID::eSlashHit, SoundEngine::CreateEventInstance(SoundID::eSlashHit) });
	myMusicList.insert({ SoundID::eWhirlwind, SoundEngine::CreateEventInstance(SoundID::eWhirlwind) });
}

AudioManager::AudioManager()
{
	myMusicVolume = 1;
	myEventVolume = 1;
}

AudioManager::~AudioManager()
{
	if (myInitialized)
	{
		myMusicList.clear();
		myBusses.clear();
		SoundEngine::Release();
	}
}
