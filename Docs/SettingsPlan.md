# Engine settings plan

## Current milestone

The JSON reader, writer, and validation are implemented for application, sound, and input settings with nlohmann/json. Startup applies the loaded values. Updates edit the requested `current` section in the existing JSON document, preserving defaults and unrelated fields, then save through a temporary file and replace the target file. The earlier hand-built writer remains in `WorkInProgress/SettingsWriting` as a local reference.

Each file replacement is atomic. Reset-all saves the two files in sequence and attempts to restore earlier values if a later step fails; it is not a single transaction across both files.

All game configurations read the single `Bin/Settings` directory. The build does not copy settings files. The startup log reports the directory in use.

## Goal

Add an `EngineSettings` service available through `ServiceLocator`. Keep two editable files in `Bin/Settings`: `ApplicationSettings.json` for application, window, and sound settings, and `InputBindings.json` for named action bindings. Each file has `defaults` and `current` sections. A category reset copies its defaults into current and applies the settings that can change during a run.

## Application and window settings

- Move the window title, windowed client resolution, window mode, `resizable` flag, render diagnostics, debug-camera mouse look, content folder path, and initial scene out of the current hardcoded configuration. The initial values should preserve today's behavior, except that the window is no longer manually resizable. Keep the 35% music default.
- Resolve the content folder path relative to the directory containing `Game.exe`. For example, `Content` names a sibling folder and `../Content` names a folder one level above. Validate that the resolved folder exists before initializing graphics and assets. Changes to this path, title, render diagnostics, and mouse look save immediately and take effect on the next run.
- Set `initialScene` in `ApplicationSettings.json` to `Blockout`, `Chests`, or `ChestMaterials`. The `current` value selects the first scene on the next run; `defaults` supplies the reset value. An older file without this field starts with `Blockout`.
- Permit `windowed` and `borderless` modes at startup and while running. Borderless fills the monitor containing the game window. Switching back restores the saved windowed client resolution and windowed style. Preserve the window's usable position where practical.
- Offer 1280x720, 1920x1080, 2560x1440, and 3840x2160 as common windowed client resolutions. The available-resolutions API returns these presets whose width and height fit within the **current pixel dimensions** of the monitor containing the window. Include the monitor's current resolution as a fallback if no preset fits. This is a list of selectable window sizes, not a list of display modes from `EnumDisplaySettings`.
- A requested windowed resolution larger than the current monitor in either dimension is capped to the largest fitting preset; persist the effective value in `current`. On a 1920x1080 monitor, 1280x720 and 1920x1080 remain available and a 4K request becomes 1920x1080. Leave the editable `defaults` value intact when capping `current`. On a later run or after moving the window to another monitor, apply the saved `current` value according to that monitor's capacity. Borderless uses monitor dimensions without replacing the stored windowed resolution.
- Set the **client area** to the requested size by calculating the outer dimensions for the chosen window style. A 1920x1080 client area plus a title bar can extend beyond a 1920x1080 desktop; keep the window reachable and document that this is expected. Update graphics targets and camera projection/aspect handling after a size change. Recheck the monitor when the window moves or display configuration changes.
- `resizable` must be `false` in both `defaults` and `current`. Reject `true` in either JSON section and in any runtime application-settings update. Disable manual sizing in the Win32 window style; programmatic resolution and mode changes remain allowed.

## Sound and input settings

- Store master, music, and SFX volumes as values from 0.0 to 1.0. Apply master and bus volumes through `AudioManager` after audio initialization and whenever they change. Keep FMOD bank paths and event definitions as they are.
- Put all named actions, including `Game` actions such as `ReloadScene`, in `InputBindings.json`. Use readable names for each device and input code, and validate unknown names. Each action has at most one code in each existing `InputMapper` map: key or mouse button, pointer motion, and gamepad. A missing or null code means unbound in that map.
- Add a per-map removal operation to `InputMapper`. Replacing or removing one code changes only that map; clearing an entire action removes its entries from all three maps. Keep listeners attached to action names during rebinding. Remove the hardcoded binds in `GameApplication` and `Game` once JSON binding initialization is in place.

## Loading, changes, and errors

- Load and validate both files before creating the window and services. Register `EngineSettings` with `ServiceLocator` early enough for game initialization and keep it alive until users of the service have shut down. Use nlohmann/json for settings reading and writing; keep input-code lookup small and explicit. Use descriptive function and variable names and short, plain comments like those in `Game.cpp` and `GameApplication.cpp`, especially around startup, apply, save, and cleanup order.
- Expose typed getters and change/reset methods for application, sound, and input categories, plus reset-all. A resolution or mode change applies while running. Volume and binding changes also apply while running. Other application fields take effect on the next run. A category reset must not overwrite unrelated categories.
- Keep the editable JSON files in `Bin/Settings` for every build configuration. Do not copy them during builds or place them under the mirrored `Content` deployment.
- Validate file shape, required fields, input names, volume ranges, window mode, positive dimensions, and both `resizable` values. Missing, malformed, or invalid files are errors rather than silently replaced. For an invalid startup file or an invalid runtime window update, include the file/field and cause in a native Windows error dialog, then perform clean shutdown. The current `GameApplication::Run` catches errors and cleans up before rethrowing, while `Main.cpp` only logs; adjust those paths so runtime errors show the dialog before cleanup begins and startup errors show it before exit, with no duplicate dialog.
- Stage and validate a change before applying it. Save each edited file through a temporary file and atomic replacement in the same directory. If writing or applying fails, keep or restore the previous in-memory and on-disk `current` state and report the failure. Do not save a requested resolution until the effective resolution is known.

## Verification

- Check both files load, malformed or invalid files produce the Windows dialog, and builds leave `Bin/Settings` unchanged. Check `resizable: true` in either JSON section and through a runtime update is rejected before shutdown.
- Check each `initialScene` choice starts the matching scene, unknown names report the field, and an older file without this field still starts with `Blockout`.
- Check all action maps, including mouse buttons in the key map. Verify changing or clearing one map leaves the other maps and listeners intact; clearing an action removes all its bindings; changes save immediately; and input reset restores defaults.
- Check master, music, and SFX volume application and category resets. Check windowed and borderless startup, runtime switching, exact client size, graphics and camera resizing, and resolution enumeration. On a 1080p monitor, verify that 720p stays available and a 4K request applies and persists as 1080p. Check behavior when the window moves to another monitor and when the configured content path changes for the next run.

## Scope

- No settings menu is included. The APIs support a future menu.
- Fullscreen means monitor-sized borderless fullscreen, without DXGI exclusive mode or a display mode switch.
