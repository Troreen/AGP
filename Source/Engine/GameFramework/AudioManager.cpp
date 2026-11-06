#include "GameFramework.pch.h"
#include "AudioManager.h"
#include "ServiceLocator.h"
#include "Settings/EngineSettings.h"

void AudioManager::Init()
{
	const std::filesystem::path bankDirectory = ServiceLocator::GetInstance().GetEngineSettings().GetExecutableDirectory() / "Audio";
	const std::string bankRootPath = bankDirectory.string() + "\\";
	SoundEngine::Init(bankRootPath);

	SoundEngine::LoadBank("Master.strings.bank", 0);
	SoundEngine::LoadBank("Master.Bank", 0);

	SoundEngine::LoadBus("bus:/SFX", "SFX");
	myBusses.insert({ BusID::eSFX, "SFX" });
	SoundEngine::LoadBus("bus:/MUSIC", "MUSIC");
	myBusses.insert({ BusID::eMusic, "MUSIC" });

	myListener = SoundEngine::GetNextFreeListener();

	SoundEngine::OverrideOtherListeners(myListener);

	RegisterAllEvents();
	myInitialized = true;
}

void AudioManager::Update(const float aDeltaTime)
{
	SoundEngine::Update();

	if (myFadeOutActive)
	{
		myMusicVolume -= myFadeSpeed * aDeltaTime;
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
		myMusicVolume += myFadeSpeed * aDeltaTime;
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
	auto instance = myMusicList.find(aMusicID);

	if (instance == myMusicList.end())
	{
		return;
	}

	if (aUninterrupted && SoundEngine::IsEventPlaying(instance->second))
	{
		return;
	}

	SoundEngine::PlayEvent(instance->second);
}

void AudioManager::PlaySFXAtLocation(const SoundID aMusicID, const CommonUtilities::Vector3f aPosition)
{
	SoundEngine::PlayEventAtLocation(myMusicList[aMusicID], { aPosition.x * 0.001f, aPosition.y * 0.001f, aPosition.z * 0.001f });
}

void AudioManager::StopMusic(const SoundID aMusicID, const bool anImmediately)
{
	SoundEngine::StopEvent(myMusicList[aMusicID], anImmediately);
}

bool AudioManager::PlayScriptedAudio(SoundID aSoundID, const CommonUtilities::Vector3f* aLocation)
{
	const auto sound = myMusicList.find(aSoundID);
	if (!myInitialized || sound == myMusicList.end())
	{
		return false;
	}

	// Every registered SoundID already has a playback. Use it directly.
	if (aLocation)
	{
		// Convert the world position to the distance scale used by our audio setup.
		return SoundEngine::PlayEventAtLocation(sound->second,
			{aLocation->x * 0.001f, aLocation->y * 0.001f, aLocation->z * 0.001f});
	}

	return SoundEngine::PlayEvent(sound->second);
}

void AudioManager::StopScriptedAudio(SoundID aSoundID)
{
	const auto sound = myMusicList.find(aSoundID);
	if (myInitialized && sound != myMusicList.end())
	{
		SoundEngine::StopEvent(sound->second, true);
	}
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

void AudioManager::SetAudioListener(const CommonUtilities::Vector3f aPosition, const CommonUtilities::Vector3f aForward, const CommonUtilities::Vector3f aUp)
{
	myListenerPosition = aPosition;

	SoundEngine::SetListenerTransform(
		myListener,
		{ aPosition.x * 0.001f, aPosition.y * 0.001f, aPosition.z * 0.001f },
		{ 0.0f, 0.0f, 0.0f },
		{ aForward.x, aForward.y, aForward.z },
		{ aUp.x, aUp.y,	aUp.z });
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
	SoundEngine::RegisterEvent("event:/Combat One", SoundID::eCombatOne);
	SoundEngine::RegisterEvent("event:/Collect Item", SoundID::eCollectItem);
	SoundEngine::RegisterEvent("event:/Crossbow Cock", SoundID::eCrossbowCock);
	SoundEngine::RegisterEvent("event:/Heavy Roar", SoundID::eHeavyRoar);
	SoundEngine::RegisterEvent("event:/Intro Swell", SoundID::eIntroSwell);
	SoundEngine::RegisterEvent("event:/Potion Drink", SoundID::ePotion);
	SoundEngine::RegisterEvent("event:/Sickle Slash", SoundID::eSickleSlash);
	SoundEngine::RegisterEvent("event:/Slash Hit", SoundID::eSlashHit);
	SoundEngine::RegisterEvent("event:/Slash", SoundID::eSlash);
	SoundEngine::RegisterEvent("event:/Ability 1", SoundID::eAbilityOne);
	SoundEngine::RegisterEvent("event:/Ability 2", SoundID::eAbilityTwo);
	SoundEngine::RegisterEvent("event:/Ability 3", SoundID::eAbilityThree);
	SoundEngine::RegisterEvent("event:/Ability 4", SoundID::eAbilityFour);
	SoundEngine::RegisterEvent("event:/MB1", SoundID::eMouseButtonOne);
	SoundEngine::RegisterEvent("event:/MB2", SoundID::eMouseButtonTwo);
	SoundEngine::RegisterEvent("event:/Footstep 1", SoundID::eFootstepOne);
	SoundEngine::RegisterEvent("event:/Dodge", SoundID::eDodge);
	SoundEngine::RegisterEvent("event:/Intro Letter", SoundID::eIntroLetter);
	SoundEngine::RegisterEvent("event:/After Heart Sway", SoundID::eAfterHeartSway);
	SoundEngine::RegisterEvent("event:/After First Kill", SoundID::eAfterFirstKill);
	SoundEngine::RegisterEvent("event:/Locked Gate", SoundID::eLockedGate);
	SoundEngine::RegisterEvent("event:/Caretaker Scream", SoundID::eCaretakerScream);
	SoundEngine::RegisterEvent("event:/Goodbye Horse", SoundID::eGoodbyeHorse);
	SoundEngine::RegisterEvent("event:/GrushenkaCrying", SoundID::eGrushenkaCrying);
	SoundEngine::RegisterEvent("event:/Reminiscing Dead Bodies", SoundID::eReminiscingDeadBodies);
	SoundEngine::RegisterEvent("event:/Meeting Daddy", SoundID::eMeetingDaddy);
	myMusicList.insert({ SoundID::eMainTheme, SoundEngine::CreateEventInstance(SoundID::eMainTheme) });
	myMusicList.insert({ SoundID::eBlizzardAmbience, SoundEngine::CreateEventInstance(SoundID::eBlizzardAmbience) });
	myMusicList.insert({ SoundID::eCastleAmbienceOne, SoundEngine::CreateEventInstance(SoundID::eCastleAmbienceOne) });
	myMusicList.insert({ SoundID::eCastleAmbienceTwo, SoundEngine::CreateEventInstance(SoundID::eCastleAmbienceTwo) });
	myMusicList.insert({ SoundID::eCombatOne, SoundEngine::CreateEventInstance(SoundID::eCombatOne) });
	myMusicList.insert({ SoundID::eCollectItem, SoundEngine::CreateEventInstance(SoundID::eCollectItem) });
	myMusicList.insert({ SoundID::eCrossbowCock, SoundEngine::CreateEventInstance(SoundID::eCrossbowCock) });
	myMusicList.insert({ SoundID::eHeavyRoar, SoundEngine::CreateEventInstance(SoundID::eHeavyRoar) });
	myMusicList.insert({ SoundID::eIntroSwell, SoundEngine::CreateEventInstance(SoundID::eIntroSwell) });
	myMusicList.insert({ SoundID::ePotion, SoundEngine::CreateEventInstance(SoundID::ePotion) });
	myMusicList.insert({ SoundID::eSickleSlash, SoundEngine::CreateEventInstance(SoundID::eSickleSlash) });
	myMusicList.insert({ SoundID::eSlashHit, SoundEngine::CreateEventInstance(SoundID::eSlashHit) });
	myMusicList.insert({ SoundID::eSlash, SoundEngine::CreateEventInstance(SoundID::eSlash) });
	myMusicList.insert({ SoundID::eAbilityOne, SoundEngine::CreateEventInstance(SoundID::eAbilityOne) });
	myMusicList.insert({ SoundID::eAbilityTwo, SoundEngine::CreateEventInstance(SoundID::eAbilityTwo) });
	myMusicList.insert({ SoundID::eAbilityThree, SoundEngine::CreateEventInstance(SoundID::eAbilityThree) });
	myMusicList.insert({ SoundID::eAbilityFour, SoundEngine::CreateEventInstance(SoundID::eAbilityFour) });
	myMusicList.insert({ SoundID::eMouseButtonOne, SoundEngine::CreateEventInstance(SoundID::eMouseButtonOne) });
	myMusicList.insert({ SoundID::eMouseButtonTwo, SoundEngine::CreateEventInstance(SoundID::eMouseButtonTwo) });
	myMusicList.insert({ SoundID::eFootstepOne, SoundEngine::CreateEventInstance(SoundID::eFootstepOne) });
	myMusicList.insert({ SoundID::eDodge, SoundEngine::CreateEventInstance(SoundID::eDodge) });
	myMusicList.insert({ SoundID::eIntroLetter, SoundEngine::CreateEventInstance(SoundID::eIntroLetter) });
	myMusicList.insert({ SoundID::eAfterHeartSway, SoundEngine::CreateEventInstance(SoundID::eAfterHeartSway) });
	myMusicList.insert({ SoundID::eAfterFirstKill, SoundEngine::CreateEventInstance(SoundID::eAfterFirstKill) });
	myMusicList.insert({ SoundID::eLockedGate, SoundEngine::CreateEventInstance(SoundID::eLockedGate) });
	myMusicList.insert({ SoundID::eCaretakerScream, SoundEngine::CreateEventInstance(SoundID::eCaretakerScream) });
	myMusicList.insert({ SoundID::eGoodbyeHorse, SoundEngine::CreateEventInstance(SoundID::eGoodbyeHorse) });
	myMusicList.insert({ SoundID::eGrushenkaCrying, SoundEngine::CreateEventInstance(SoundID::eGrushenkaCrying) });
	myMusicList.insert({ SoundID::eReminiscingDeadBodies, SoundEngine::CreateEventInstance(SoundID::eReminiscingDeadBodies) });
	myMusicList.insert({ SoundID::eMeetingDaddy, SoundEngine::CreateEventInstance(SoundID::eMeetingDaddy) });
}

AudioManager::AudioManager()
{

}

AudioManager::~AudioManager()
{
	if (myInitialized)
	{
		StopAllEvents();

		myMusicList.clear();
		myBusses.clear();

		SoundEngine::Release();

		myInitialized = false;
	}
}
