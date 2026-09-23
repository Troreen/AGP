#pragma once
#include "fmod.h"
#include "SoundEngine.h"
#include "Vector3.hpp"

enum SoundID
{
	eMainTheme,
	eBlizzardAmbience,
	eCastleAmbienceOne,
	eCastleAmbienceTwo,
	eCollectItem,
	eCrossbowCock,
	eHeavyRoar,
	eIntroSwell,
	ePotion,
	eSickleSlash,
	eSlashHit,
	eWhirlwind
};

enum BusID
{
	eSFX,
	eMusic,
};

class AudioManager
{
public:
	AudioManager();
	~AudioManager();
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	void Init();
	void Update(const float aDeltaTime);

	void PlaySFX(const SoundID aSoundID);
	void PlayMusic(const SoundID aMusicID, const bool aUninterrupted = false);

	void PlaySFXAtLocation(const SoundID aMusicID, const CommonUtilities::Vector3f aPosition);

	void StopMusic(const SoundID aMusicID, const bool anImmediately);
	void StopAllEvents();

	const bool IsEventPlaying(const SoundID aMusicID);

	void SetBusVolume(BusID anID, float aPercentage);
	void SetMasterVolume(float aPercentage);
	void SetEventVolume(const SoundID aSoundID, float aPercentage);

	void SetAudioListener(const CommonUtilities::Vector3f aPosition);
	const CommonUtilities::Vector3f& GetAudioListenerPosition() const;

	void FadeOut(const SoundID anID, const float aFadeSpeed);
	void FadeIn(const SoundID anID, const float aFadeSpeed);

	void ResetSounds();

private:
	void RegisterAllEvents();

	std::unordered_map<SoundID, SoundEventInstanceHandle> myMusicList;
	std::unordered_map<BusID, std::string> myBusses;
	SoundEngine::ListenerHandle myListener;

	bool myFadeOutActive;
	bool myFadeInActive;
	float myFadeSpeed;
	float myMusicVolume;
	static inline float myMasterVolume;
	static inline float myBusVolume[2];
	float myEventVolume;
	SoundID myFadeID;
	bool myInitialized = false;

	CommonUtilities::Vector3f myListenerPosition;
};

