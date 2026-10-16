# Positional audio diagnosis

Reported symptom: the scripted positional sound sounds unchanged when moving farther away.

Confirmed integration defect: AudioManager creates a listener and provides SetAudioListener, but no gameplay code called that setter. The listener therefore stayed at its initial position when the player or debug camera moved. Game::Update now sends the active camera's current world position. Listener placement is game policy; GameApplication only advances the audio service. No camera pointer is retained across scene loads. Game::Update runs before World::Update, so movement applied during the World update reaches the listener on the following frame.

Checked the actual shipped FMOD bank with the offline probe:

```powershell
& 'C:/Users/tarik.bergstrom/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' .scratch/scripted-events/probe_spatial_audio.py
```

The probe loads Bin/Debug/Audio banks through the FMOD DLLs, starts Heavy Roar, and meters the real master output with no speaker output. It verifies nonzero near playback and lower far playback. Heavy Roar reports Is3D=true, minimum distance 1, maximum distance 20. One run measured RMS approximately 0.065 at distance 0, 0.022 at distance 10, and 0 at distance 1000. The event's samples can vary, so compare attenuation rather than exact RMS values.

This rules out an entirely non-spatial or non-attenuating roar asset. It does not reproduce the full application frame loop. There is no existing unattended audio/camera integration test seam in GameApplicationTests; no source-text assertion or mocked volume calculation was added as a substitute for an audible check.

The existing world-to-audio conversion multiplies source and listener positions by 0.001. Therefore a 1,000-unit move equals one FMOD unit. That scale is preserved in this fix. Heavy Roar starts at the target position captured on action start and does not follow the moving target. Collect Item is deliberately non-positional in the test sequence.

Remaining manual check: restart the rebuilt Game.exe, play with F2 near the source, move the active debug camera several thousand engine units away, and replay with F2. Compare the roar, not Collect Item. Check F4 scene replacement and camera switching as well. Audibility of the complete application has not been verified by the offline bank probe.
