# Input restoration

Implemented from `6d730498ba9b41e3f7ef3bf6dc173cebb6d510c7`. The previous bridge proposal is superseded.

`Win32 window procedure -> InputHandler -> InputMapper::Update -> listeners -> game/components`

The historical InputMapper header and implementation are restored byte-for-byte. Each input user calls `BindActionToInputCode` and `AddEventListener` directly. Components keep unsigned listener IDs and remove them in `EndPlay`; Game removes its IDs in `Shutdown`. Runtime actor changes occur after dispatch. World and GameContext no longer own or forward input services.

ServiceLocator retains the historical explicit singleton design. `SetInputMapper` transfers ownership (and deletes a replaced mapper); `KillServices` deletes the mapper and clears all service pointers. Register/replace the mapper only when no old listeners remain. The mapper borrows the runtime's InputHandler and XInputHandler. AudioManager and AssetRegistry are borrowed: AudioManager shuts down through its own API, and AssetRegistry has static lifetime.

The runtime clears the world before deleting services, including initialization/update/shutdown failure paths. It calls mapper Update exactly once per rendered frame. Mouse centering only resets the handler's position baseline; it does not generate a separate action stream. Focus-loss messages clear tentative held keys so the mapper emits releases on its next update.

## Historical mapper constraints

- One binding per action per device category; another key replaces the existing key. There are no alternate-key controls in the current game.
- Current bindings are limited to camera movement/mouse look and F1/F4/F5/F6/F7/F8. Other input bindings and the animation/light input components were removed in the follow-up cleanup. Spin remains automatic.
- Listener dispatch order is unspecified. Do not add/remove listeners, mutate bindings, destroy listeners' owners, or recursively update the mapper during a callback. Copy data needed later; InputEvent borrows stack InputData.
- ClearBindingsFromAction removes only the first matching device category. The runtime does not rely on it.
- Gamepad analog previous-state flags are shared per physical axis; disconnect does not emit releases. No default gamepad mappings are installed, and controller behavior has not been validated on hardware.
- Press and release within the same pumped frame can collapse to no edge because InputHandler stores final key state. Focus regain does not resample held keyboard keys.

This change does not implement the broader GameFramework refactor.

## Validation (Debug x64)

Before source edits: AGP solution built successfully. Framework tests built but failed `Missing material parent was accepted`; runtime tests did not compile because their scene API calls were stale. Optimization tests and public gameplay consumer built; optimization tests passed. MSBuild requires execution outside the filesystem sandbox because Visual Studio FileTracker otherwise returns access denied.

After migration:

- `AGP.sln` final Debug build passed (engine libraries and Game).
- GameFrameworkTests, GameRuntimeTests, and PublicGameplayConsumer Debug builds passed; runtime test scene calls were migrated to the existing SceneType API.
- Public header isolation passed for all 17 gameplay headers.
- `GameFrameworkTests.exe --input-only` passed: native press/hold/release, mouse deltas/config gate, focus loss, explicit removal, repeated world cleanup, debug camera, mapper replacement, borrowed assets and idempotent service cleanup.
- `GameRuntimeTests.exe camera-controls` and `render-pass-controls` passed: camera yaw/pitch/clamping, light Shift/alternate-key behavior, frame-time spinning and listener destruction, F5/F6.
- Graphics scenarios `sample`, `chest-materials`, `text-overlay`, `invalid-initial`, `initialize-failure`, `begin-failure`, `component-failure`, `update-failure`, and `shutdown-failure` passed with clean D3D debug queues. The sample covers rejected candidates, successful reload and an empty world. Shutdown checks assert mapper null and audio/assets unavailable.
- Both InputMapper files compare byte-for-byte equal to the historical Git blobs. Source and test searches contain no replacement input API identifiers; only InputMapper calls device UpdateInput in production.
- Independent review checked service ownership, dispatch mutation, focus delivery and mouse configuration; its findings were fixed.

Graphics scenarios ran with `Bin/Debug` as working directory using its existing staged Content. Source Content lacks `Shaders/Internal/Tonemap_PS.hlsl`, so running those scenarios from the repository root remains blocked. Staged shader files were not restored into source as part of this input change. The full framework test retains its baseline material-parent failure. Existing missing TGA_Bro mesh/animation warnings prevent claiming successful animation playback validation.

Interactive verification was attempted with the computer-use skill but stopped by the user's physical Escape key. No further UI actions were issued; manual movement/mouse-warp/animation confirmation and physical-controller verification remain incomplete. Only Debug was built.

## Follow-up control simplification

Removed animation/light input components, tonemapper controls, R spin toggling, P diagnostics, and Escape quitting. Camera controls and F1/F4/F5/F6/F7/F8 remain; close the window to quit. Earlier light/spin-toggle validation above describes the initial migration, before this simplification.

Follow-up validation: Debug AGP solution, GameRuntimeTests and PublicGameplayConsumer builds passed. Camera/automatic-spin and F5/F6 regression tests passed.
