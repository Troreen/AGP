/**
 * SoundEngine wrapper for FMOD
 * ----------------------------
 * By Daniel Borgshammar & Kristian Svensson @ The Game Assembly
 *
 * Intended as a lightweight wrapper to FMOD and to help students learn how FMOD works.
 * At least the basics of it :).
 *
 * *** Please email daniel.borgshammar@thegameassembly.se (or page on teams) if you find
 * *** any issues!
 *
 * 22/03/04 - Ver 1.0 Release
 *		Probably has a bunch of interesting bugs in it but testing it seems to suggest
 *		it's working well enough to release. No outstanding issues at this time.
 *		More features are in the works for the future.
 * 
 * 23/04/28 - Ver 1.1 Release
 *		Added functionality for: 
 *			-Busses/Groups can now be loaded from FMOD Stuidio
 *			-Bus volume control.
 *			-A check for IsEventPlaying
 *			-Listener bugs are fixed
 *			-Listeners can now overload eachothers weight values.
 *			-Listeners can now be moved and placed with SetListenerPosition.
 *			-Events can now be played at location just as PlaySoundAtLocation.
 *	- Kristian Svensson SP22
 */

#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <array>
#include <unordered_map>

#include "SoundEngineStructs.h"

struct SoundEngineImpl;

class SoundEngine
{
	SoundEngine() = default;
	~SoundEngine() = default;

	// PIMPL idiom and not an object or singleton as such.
	// Allows cleaner interface by using static functions but
	// uses controlled instantiation through Init.
	// Also allows the FMOD code to live individually while the
	// SoundEngine specifics are contained in this class.
	static inline SoundEngineImpl* myImpl = nullptr;

	static inline std::unordered_map<unsigned int, std::string> myEventAccelMap{};

public:
	struct ListenerHandle;

	static SoundEngineImpl* GetImpl() { return myImpl; }

private:

	// Defines a spatial listener at a relative position.
	struct Listener
	{
		int ListenerId = 0;
		int FmodId = -1;
		std::array<float, 3> Location{0, 0, 0};
		std::array<float, 3> Velocity{0, 0, 0};
		std::array<float, 3> Forward{0, 0, 1};
		std::array<float, 3> Up{0, 1, 0};

		struct ListenerHandle Handle() const;
	};

	static inline std::unordered_map<int, Listener> myListeners{};
	static inline int nextListenerId = 0;

	static bool RegisterCallbackInternal(const SoundEventInstanceHandle& anEventInstanceHandle, std::shared_ptr<EventCallbackBase> aPtr);

public:

	// Represents a specific listener
	struct ListenerHandle
	{
		friend struct Listener;
	private:
		int myId = -1;
	public:
		int GetId() const;
		bool IsValid() const;
	};


public:

	/**
	 * Initializes and configures the SoundEngine. Sets the root path where we'll look for files to load.
	 * @param aRootPath The root path where LoadBank and LoadSoundFile will look for its files. Can be relative or absolute.
	 * @returns True if the initialization was successful.
	 */
	static bool Init(const std::string& aRootPath);

	/**
	 * Releases all resources involved in the SoundEngine.
	 */
	static void Release();

	/**
	 * Frame update, allows the SoundEngine to process commands.
	 */
	static void Update();

	/**
	 * Loads a sound file and readies for playback.
	 * @param aFileName The file name of the sound file to load.
	 * @param enable3d If True the file will respond to 3D positioning of sounds, useful for files that will be played in a 3D world.
	 * @param shouldStream If the file will be streamed from storage or fully loaded into memory.
	 * @parma shouldLoop If the file should loop when played back or not.
	 */
	static bool LoadSoundFile(const std::string& aFileName, bool enable3d = false, bool shouldStream = false, bool shouldLoop = false);

	/**
	 * Plays a sound previously loaded with LoadSoundFile. Will load the file on the fly if it's not preloaded. May cause a hitch!
	 * @param aFileName The file name of the sound file to play.
	 * @param aVolume The volume to set when playing the file.
	 * @returns The channel number of this sound.
	 */
	static size_t PlaySound(const std::string& aFileName, const float aVolume = 1.0f);

	/**
	 * Stops a sound channel that was previously stated with PlaySound.
	 * @param aChannelNumber The channel number created by PlaySound that you would like to stop.
	 * @returns True if the channel was successfully stopped (or already was stopped).
	 */
	static bool StopSound(size_t aChannelNumber);

	/**
	 * Restart a sound channel. Will also restart currently playing channels.
	 * @param aChannelNumber The channel number created by PlaySound that you would like to restart.
	 * @returns True if the channel was successfully restarted.
	 */
	static bool RestartSound(size_t aChannelNumber);

	/**
	 * Plays a sound previously loaded with LoadSoundFile at the specified 3D coordinates. Will load the file on the fly if it's not preloaded. May cause a hitch!
	 * If the file was not loaded with enable3d = true this will have the same effect as PlaySound.
	 * @param aFileName The file name of the sound file to play.
	 * @param aVolume The volume to set when playing the file.
	 * @param aPosition Relative position at which to play the sound.
	 * @returns The channel number of this sound.
	 */
	static size_t PlaySoundAtLocation(const std::string& aFileName, const float aVolume = 1.0f, std::array<float, 3> aPosition = {0, 0, 0});

	/**
	 * Loads the specified bank into memory and readies it for play.
	 * @param aFileName The FMOD bank to load.
	 * @param someFlags FMOD bank loading flags.
	 * @returns True if the bank was successfully loaded.
	 */
	static bool LoadBank(const std::string& aFileName, unsigned int someFlags);

	/**
	 * Loads the specified bus into memory and readies it for play.
	 * @param aBusPath The FMOD bus to load.
	 * @param anID The name/ID of your loaded bus (This can be whatever you like).
	 * @returns True if the bus was successfully loaded.
	 */
	static bool LoadBus(const std::string& aBusPath, const std::string& anID);

	/**
	 * Loads a specific event using its provided path.
	 * @param anEventName The FMOD event path for this event.
	 * @returns True if the event was successfully loaded.
	 */
	static bool LoadEvent(const std::string& anEventName);

	/**
	 * Fetches the list of events that have been loaded from all banks.
	 * @param outEventList The list of event paths to populate.
	 * @returns The number of events found.
	 */
	static unsigned GetEvents(std::vector<std::string>& outEventList);

	/**
	 * Registers an event to event ID. Intended for acceleration structures such as mapping an enum or namespace integer to an Event Name.
	 * Allows similar usage to WWISE headers, i.e.:
	 * MyNamespace::MyEvents::MyEventName
	 * @param anEventName The event string name to bind.
	 * @param anEventId The event ID (i.e. MyNamespace::MyEvents::MyEventName) to bind the event to.
	 * @returns True if the binding was successful. False if the binding already exists.
	 */
	static bool RegisterEvent(const std::string& anEventName, unsigned int anEventId);

	/**
	 * Creates an instance of the specified event, allowing it to be controlled independently.
	 * If you only need to play the event once you can use PlayEventOneShot as a fire and forget instead.
	 * @param anEventName The name of the event to instantiate.
	 * @returns A valid handle if the event was successfully instantiated. Check with IsValid to determine validity.
	 */
	static SoundEventInstanceHandle CreateEventInstance(const std::string& anEventName);

	/**
	 * Creates an instance of the specified event, allowing it to be controlled independently.
	 * If you only need to play the event once you can use PlayEventOneShot as a fire and forget instead.
	 * @param anEventId The event ID to instantiate.
	 * @returns A valid handle if the event was successfully instantiated. Check with IsValid to determine validity.
	 */
	static SoundEventInstanceHandle CreateEventInstance(unsigned int anEventId);

	/**
	 * Creates an instance of the specified event, allowing it to be controlled independently.
	 * If you only need to play the event once you can use PlayEventOneShot as a fire and forget instead.
	 * @param anEventGUID The FMOD GUID of the event to play.
	 * @returns A valid handle if the event was successfully instantiated. Check with IsValid to determine validity.
	 */
	static SoundEventInstanceHandle CreateEventInstance(const FMOD_GUID& anEventGUID);

	/**
	 * Instructs an event instance to play.
	 * @param anEventInstanceHandle The instance of the event to play.
	 * @returns True if the event was successfully triggered.
	 */
	static bool PlayEvent(const SoundEventInstanceHandle& anEventInstanceHandle);
	
	/**
	 * Instructs an event instance to play. at the specified 3D coordinates.
	 * @param anEventInstanceHandle The instance of the event that is playing.
	 * @param aPosition Relative position at which to play the sound.
	 * @returns True if the event is currently playing, otherwise it returns false.
	 */
	static bool PlayEventAtLocation(const SoundEventInstanceHandle& anEventInstanceHandle, std::array<float, 3> aPosition = { 0, 0, 0 });

	/**
	 * Checks wether an Event is currently playing.
	 * @param anEventInstanceHandle The instance of the event that is playing.
	 * @returns True if the event is currently playing, otherwise it returns false.
	 */
	static bool IsEventPlaying(const SoundEventInstanceHandle& anEventInstanceHandle);

	/**
	 * Fetches the channel group of the specified event instance. This only works if the event is fully loaded!
	 * @param anEventInstanceHandle The instance of the event.
	 * @returns The ChannelGroupHandle of the channel group the event belongs to.
	 */
	static ChannelGroupHandle GetEventInstanceChannelGroup(const SoundEventInstanceHandle& anEventInstanceHandle);

	/**
	 * Plays an event once by creating a new instance, playing it, and then destructing it when it has finished.
	 * param anEventGUID The FMOD GUID of the event to play.
	 * @returns True if the event was successfully triggered.
	 */
	static bool PlayEventOneShot(const FMOD_GUID& anEventGUID);

	/**
	 * Plays an event once by creating a new instance, playing it, and then destructing it when it has finished.
	 * @param anEventName The FMOD event to play.
	 * @returns True if the event was successfully triggered.
	 */
	static bool PlayEventOneShot(const std::string& anEventName);

	/**
	 * Plays an event once by creating a new instance, playing it, and then destructing it when it has finished.
	 * @param anEventId The event ID to instantiate.
	 * @returns True if the event was successfully triggered.
	 */
	static bool PlayEventOneShot(unsigned int anEventId);

	/**
	 * Stops an active event from a loaded Bank.
	 * @param anEventInstanceHandle The instance of the event to play.
	 * @param immediately If the evente should be forced to stop of stop after it has finished processing.
	 * @returns True if the event was successfully stopped.
	 */
	static bool StopEvent(const SoundEventInstanceHandle& anEventInstanceHandle, bool immediately);

	/**
	 * Fetches data from the specified Event Parameter.
	 * @param anEventInstanceHandle The instance that contains the parameter.
	 * @param aParameterName The parameter to fetch the value of.
	 * @param outValue A valid float pointer to store the value in.
	 * @returns True if the value was successfully retrieved.
	 */
	static bool GetEventParameter(const SoundEventInstanceHandle& anEventInstanceHandle, const std::string& aParameterName, float* outValue);

	/**
	 * Sets the value of the specified Event Parameter.
	 * @param anEventInstanceHandle The instance that contains the parameter.
	 * @param aParameterName The parameter to set the value of.
	 * @param aValue The new value to set.
	 * @returns True if the value was successfully set.
	 */
	static bool SetEventParameter(const SoundEventInstanceHandle& anEventInstanceHandle, const std::string& aParameterName, float aValue);

	/**
	 * Sets the value of the specified Global Parameter.
	 * @param aParameterName The parameter to set the value of.
	 * @param aValue The new value to set.
	 * @param ignoreSeekSpeed If we should ignore the parameters seek speed and set the value immediately.
	 * @returns True if the value was successfully set.
	 */
	static bool SetGlobalParameter(const std::string& aParameterName, float aValue, bool ignoreSeekSpeed = false);

	/**
	 * Fetches data from the specified Global Parameter.
	 * @param aParameterName The parameter to fetch the value of.
	 * @param outValue A valid float pointer to store the value in.
	 * @returns True if the value was successfully retrieved.
	 */
	static bool GetGlobalParameter(const std::string& aParameterName, float* outValue);

	/**
	 * Sets the master volume for all channels.
	 * @param aVolumePct The volume in percent, a float ranging from 0-1
	 */
	static bool SetMasterVolume(float aVolumePct);

	/**
	 * Sets the master volume for all channels.
	 * @param aVolumeDB The volume in percent, a float ranging from 0-1
	 */
	static bool SetMasterVolumeDB(float aVolumeDB);

	/**
	 * Sets the volume for a specified Bus.
	 * @param anID The ID of the specified Bus 
	 * @param aVolumePct The volume in percent, a float ranging from 0-1
	*/
	static bool SetBusVolume(const std::string& anID, float aVolumePct);

	/**
	 * Sets the volume for a specified Bus.
	 * @param anID The ID of the specified Bus 
	 * @param aVolumeDB The volume in Decibels.
	*/
	static bool SetBusVolumeDB(const std::string& anID, float aVolumePctDB);

	/**
	 * Sets the volume for the specified sound channel index.
	 * @param aChannel The sound channel index.
	 * @param aVolumePct The volume in percent, a float ranging from 0-1
	 */
	static bool SetVolume(unsigned int aChannel, float aVolumePct);

	/**
	 * Sets the volume for the specified sound channel index.
	 * @param aChannel The sound channel index.
	 * @param aVolumeDB The volume in Decibels.
	 */
	static bool SetVolumeDB(unsigned int aChannel, float aVolumeDB);

	/**
	 * Sets the volume for the specified event.
	 * @param anEventInstanceHandle The instance of the event to set the volume for.
	 * @param aVolumePct The volume in Decibels.
	 */
	static bool SetEventVolume(const SoundEventInstanceHandle& anEventInstanceHandle, float aVolumePct);

	/**
	 * Sets the volume for the specified event.
	 * @param anEventInstanceHandle The instance of the event to set the volume for.
	 * @param aVolumeDB The volume in Decibels.
	 */
	static bool SetEventVolumeDB(const SoundEventInstanceHandle& anEventInstanceHandle, float aVolumeDB);

	/**
	 * Sets the volume for the specified sound channel group.
	 * @param aGroupHandle The sound channel group handle.
	 * @param aVolumePct The volume in percent, a float ranging from 0-1
	 */
	static bool SetChannelGroupVolume(const ChannelGroupHandle& aGroupHandle, float aVolumePct);

	/**
	 * Sets the volume for the specified sound channel group.
	 * @param aGroupHandle The sound channel group handle.
	 * @param aVolumeDB The volume in Decibels.
	 */
	static bool SetChannelGroupVolumeDB(const ChannelGroupHandle& aGroupHandle, float aVolumeDB);

	/**
	 * Sets the listener mask for the specified event. Note that a listener mask should be
	 * bitshifted together for it to work.
	 * To make a mask that allows listeners 0 and 2: mask = (1 << 0) | (1 << 2).
	 * @param anEventInstanceHandle The instance of the event to set the listener mask for.
	 * @param aMask The mask to set.
	 */
	static bool SetEventListenerMask(const SoundEventInstanceHandle& anEventInstanceHandle, unsigned int aMask = 0xFFFFFFFF);

	/**
	 * Sets the 3D parameters of this event.
	 * @param anEventInstanceHandle The instance of the event to set the 3D parameters for.
	 * @param aPosition The position of the event.
	 * @param aVelocity The event velocity.
	 * @param aForwardVector The forward vector in the event rotation.
	 * @param aUpVector The up vector in the event rotation.
	 */
	static bool SetEvent3DParameters(const SoundEventInstanceHandle& anEventInstanceHandle, std::array<float, 3> aPosition = {0, 0, 0}, std::array<float, 3> aVelocity = {0, 0, 0}, std::array<float, 3> aForwardVector = {0, 0, 1}, std::array<float, 3> aUpVector = {0, 1, 0});

	/**
	 * Sets the Pitch of this event.
	 * @param anEventInstanceHandle The instance of the event to set the pitch for.
	 * @param aPitch The new pitch.
	 */
	static bool SetEventPitch(const SoundEventInstanceHandle& anEventInstanceHandle, float aPitch);

	/**
	 * Sets the Pitch of this event.
	 * @param anEventInstanceHandle The instance of the event to set the pitch for.
	 * @param anIndex The Reverb instance index.
	 * @param aLevel The new reverb level.
	 */
	static bool SetEventReverbLevel(const SoundEventInstanceHandle& anEventInstanceHandle, int anIndex, float aLevel);

	/**
	 * Retrieves the next free listener if there is one available. FMOD allows a maximum of 8 simultaneous listeners including the default listener at Origin.
	 * @returns A handle to the new Listener. Check ListenerHandle.IsValid() to determine the result.
	 */
	static ListenerHandle GetNextFreeListener();

	/**
	* Sets the weight of all other listeners in SoundEngine to 0 and aHandle Listener to 1.
	* @param aHandle The listener that you wish to make override all others.
	*/
	static bool OverrideOtherListeners(ListenerHandle aHandle);

	/**
	 * Removes a 3D listener from the system.
	 * @param aHandle The listener handle to work with.
	 * @returns True if the listener could be removed.
	 */
	static bool RemoveListener(ListenerHandle aHandle);

	/**
	 * Sets the RELATIVE position of the specified listener using a vector3 equivalent.
	 * @param aHandle The listener handle to work with.
	 * @param aPosition The new listener position
	 * @returns True if the change was successful.
	 */
	static bool SetListenerPosition(ListenerHandle aHandle, std::array<float, 3> aPosition);

	/**
	 * Sets the RELATIVE transform of the specified listener using individual vector3 equivalents.
	 * @param aHandle The listener handle to work with.
	 * @param aPosition The new listener position.
	 * @param aVelocity The new listener velocity.
	 * @param aForwardVector The new listener Forward vector.
	 * @param aUpVector The new listener Up vector.
	 * @returns True if the change was successful.
	 */
	static bool SetListenerTransform(ListenerHandle aHandle, const std::array<float, 3> aPosition = {0, 0, 0}, const std::array<float, 3> aVelocity = {0, 0, 0}, const std::array<float, 3> aForwardVector = {0, 0, 0}, const std::array<float, 3> aUpVector = {0, 0, 0});

	/**
	 * Sets the RELATIVE transform of the specified listener using a matrix equivalent.
	 * Matrix is expected to be in Row-Major format, just like DirectX expects. If your matrix is not in Row-Major you can transpose it since it's uniform.
	 * @param aHandle The listener handle to work with.
	 * @param aTransform The transform matrix in array order.
	 * @parma aVelocity The listener velocity.
	 * @returns True if the change was successful. May return false if the matrix is not well formed.
	 */
	static bool SetListenerTransform(ListenerHandle aHandle, const std::array<float, 16> aTransform, const std::array<float, 3> aVelocity = {0, 0, 0});

	/**
	 * Registers a callback for the specified event at the specified moments (see CALLBACKTYPE)
	 * @param anEventInstanceHandle The instance of the event to register for.
	 * @param aCallbackType A combination of CALLBACKTYPEs. I.e. CALLBACKTYPE_STARTED or CALLBACKTYPE_STOPPED | CALLBACKTYPE_STARTED or CALLBACKTYPE_ANY
	 * @param aPtr The object to call the callback on when it triggers.
	 * @param aFunc The function to call on the object, i.e. &MyClass::MyFunc. Must have the signature FuncName(CALLBACKTYPE, const std::string&)
	 * @returns True if the callback was successfully registered.
	 */
	template<class Class>
	static bool RegisterCallbackRaw(const SoundEventInstanceHandle& anEventInstanceHandle, int aCallbackType, std::shared_ptr<Class> aPtr, typename SoundEngineCallback<Class>::Raw aFunc)
	{
		std::shared_ptr<EventCallbackRaw<Class>> callbackPtr(new EventCallbackRaw<Class>(aPtr, aCallbackType, aFunc));
		return RegisterCallbackInternal(anEventInstanceHandle, callbackPtr);
	}

	/**
	 * Registers a callback for the specified event at the specified moments (see CALLBACKTYPE)
	 * @param anEventInstanceHandle The instance of the event to register for.
	 * @param aCallbackType A combination of CALLBACKTYPEs. I.e. CALLBACKTYPE_STARTED or CALLBACKTYPE_STOPPED | CALLBACKTYPE_STARTED or CALLBACKTYPE_ANY
	 * @param aLambda A lambda function to register as a callback. Must have the signature FuncName(CALLBACKTYPE, const std::string&).
	 * @returns True if the callback was successfully registered.
	 */
	static bool RegisterCallbackLambda(const SoundEventInstanceHandle& anEventInstanceHandle, int aCallbackType, EventCallbackLambda::Lambda aLambda);

	/**
	 * If something or other has gone wrong you can use this function to retrieve the latest error code for more information.
	 */
	static SERESULT GetLastError();
};