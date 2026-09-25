# Windows GitHub Portability TODO

Status: in progress. This document records work to make a fresh GitHub checkout buildable and a Release package runnable on another supported x64 Windows PC.

## Current blockers

- `GenerateProject.bat` now downloads and verifies Premake 5.0.0-beta8 when the executable is missing. A clean-clone run still needs verification.
- `Source/Application/Game/premake5.lua` copies five DLLs from `Dependencies/.dlls` before building Game. Those exact files are now tracked in Git; their original sources and redistribution rights still need review. The single tracked `Bin/Debug/libfbxsdk.dll` is not a complete dependency source.
- The workspace selects the `v145` C++ toolset in `premake5.lua`. A new development machine needs Visual Studio 2026 C++ build tools and a Windows SDK. See [Microsoft's MSVC toolset documentation](https://learn.microsoft.com/en-us/cpp/overview/what-s-new-for-msvc?view=msvc-170).
- Git currently tracks a snapshot of `Content`, while the team updates Content through a separate Perforce stream. A GitHub clone does not automatically receive later Perforce updates.
- `AGPCleaner.bat` packages Debug by default. Debug Visual C++ runtime DLLs are [not generally redistributable](https://learn.microsoft.com/en-us/cpp/windows/preparing-a-test-machine-to-run-a-debug-executable?view=msvc-170).
- `README.md` still describes the removed `Assets` runtime layout rather than executable-relative `Content`.

## Decisions before implementation

- [ ] Decide whether an approved, versioned Content snapshot belongs in GitHub or whether a documented Perforce/content-package sync is required. State plainly when a GitHub checkout alone is insufficient.
- [ ] Identify the exact sources and versions of Premake, FMOD runtime DLLs, and the FBX SDK runtime DLL. Premake is pinned and runtime DLL bytes are recorded in `Dependencies/.dlls/README.md`. `SetupWindows.bat` guides developers through installing the required FMOD 2.02.05 SDK headers from FMOD's download page.
- [ ] Review redistribution rights for third-party SDK files, runtime DLLs, and Content before publishing the repository or a package. In particular, [FMOD's licence](https://fmod.com/legal) distinguishes SDK files from runtime libraries. FMOD vendor headers are now obtained through `SetupWindows.bat`; the team-provided `SoundEngine` wrapper headers and libraries remain tracked.

## Implementation checklist

### Fresh-clone build

- [x] Supply a pinned Premake executable through an approved repository file or bootstrap download, including its licence and an integrity check. Make `GenerateProject.bat` fail with a useful message when Premake is unavailable. `GenerateProject.bat` now downloads the official 5.0.0-beta8 Windows archive when needed and verifies both the archive and executable SHA-256 hashes. The licence is at `Premake/LICENSE.txt`.
- [ ] Confirm the sources and redistribution rights of the five runtime DLLs now tracked in `Dependencies/.dlls`, then verify that a clean clone builds from those files rather than an old checkout.
- [ ] Add a preflight check that reports missing toolchain components, Content, and DLLs before compiling.
- [ ] Document the required Visual Studio 2026 Desktop development with C++ workload, `v145` toolset, Windows SDK, and `Debug/Release | x64` build commands.

### Portable Release runtime

- [x] Resolve audio bank paths relative to the executable or configured runtime root, independent of the working directory.
- [x] Deploy the FMOD banks with the executable, alongside the existing `Bin/Release/Content` output.
- [ ] Make the app-only package use Release and include `Game.exe`, `Content`, audio banks, and the required Release FMOD/FBX DLLs. Verify that no path inside the package points back to the source checkout.
- [ ] Document or install the x64 Visual C++ Redistributable version required by the build. See [Microsoft's supported redistributable guidance](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170). Keep Debug builds for development and testing.
- [ ] Update `README.md` with separate instructions for building from source and running a Release package.

### Verification

- [ ] Test from a fresh GitHub clone on a second Windows PC or clean VM with no previous AGP build outputs. Run `GenerateProject.bat`, then clean Debug and Release x64 builds.
- [ ] Verify that the build automatically places fonts, scenes, materials, `TextOverlay_VS.hlsl`, `TextOverlay_PS.hlsl`, `BRDF_LUT_PS.hlsl`, and the other engine shaders beside `Game.exe` under `Content`.
- [ ] Run the Release package from an unrelated working directory without access to the original checkout or Perforce workspace. Verify scene loading, text rendering, shader loading, and audio playback.
- [ ] Add a repeatable Windows CI clean-clone build and packaging check after the dependency acquisition method is settled.

Done means another supported x64 Windows PC can follow the documented setup, build the source, and run the Release package without undocumented local files.
