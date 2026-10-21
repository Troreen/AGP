# Home setup and working with AGP content

AGP contains the engine. AGP-Content contains the Unreal project and game assets.
At home, clone both repositories and copy the runtime content into AGP when needed.
At school, keep using your existing manual Git/Perforce workflow.

## What belongs where

| Files | Git repository |
| --- | --- |
| Engine C++ code, engine shader sources, build configuration, engine settings | AGP |
| Unreal maps/materials/assets, game material shaders, exported scenes, runtime FBX/DDS assets | AGP-Content |
| `AGP/Content/` | Local runtime copy; ignored by AGP's Git |
| `AGP/Bin/<configuration>/EngineShaders/` | Generated during the engine build |

Edit shared assets in AGP-Content. An edit made only inside `AGP/Content/`
will not be included when committing AGP. Copy any such edit back to the matching
AGP-Content file and review it before syncing content again.

## Set up the home PC

### 1. Install the tools

- Git and Git LFS, with access to both GitHub repositories.
- Visual Studio 2026 with Desktop development with C++, the v145 toolset,
  and a Windows SDK for building AGP.
- Unreal Engine 5.7 if you want to edit levels or Unreal assets.
- FMOD Studio API 2.02.05 headers for building AGP; `SetupWindows.bat`
  explains where to obtain them and asks for the downloaded SDK folder.
- `texconv.exe` beside `AGP-Content/ExportAssets.bat` if you need to convert new
  PNG textures. The converter is part of Microsoft's DirectXTex tools and is
  not currently included in the repository. Existing committed DDS assets
  can be used without running conversion again.

### 2. Clone the repositories beside each other

Run these commands in PowerShell. Change `C:\Development` if desired.

```powershell
New-Item -ItemType Directory -Path C:\Development -Force
Set-Location C:\Development
git lfs install
git clone https://github.com/Troreen/AGP.git
git clone https://github.com/Troreen/AGP-Content.git
git -C .\AGP lfs pull
git -C .\AGP-Content lfs pull
```

Expected layout:

```text
C:\Development\
  AGP\
    Source\
    Docs\
  AGP-Content\
    Content\
    Unreal\
    ExportAssets.bat
    SyncContent.ps1
```

If the clones already exist, update them using your usual Git workflow rather
than cloning over them. Save local work first. An older AGP checkout receiving
the commit that removes content from Git may lose those tracked local files;
run the copy step below to populate `Content/` again.

### 3. Supply AGP with runtime content

Run from `C:\Development\AGP-Content`:

```powershell
Set-Location C:\Development\AGP-Content
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\SyncContent.ps1 -WhatIf
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\SyncContent.ps1
```

The default destination is the sibling `AGP` folder. For another layout, add
`-EngineRoot "D:\YourFolder\AGP"` to each command.

The first command previews changed/missing files. The second copies them.
Identical files are skipped. If existing destination files differ, copying
stops before writing. Review those differences and preserve any local edits;
then explicitly allow replacement when appropriate:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\SyncContent.ps1 -OverwriteChangedFiles
```

Copying does not delete destination-only files. Clean up obsolete runtime files
manually when removing or renaming assets. To leave a specific local file alone,
pass its exact path relative to `Content/`, for example:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\SyncContent.ps1 -ExcludeRelativePath 'ExportedScenes/lvl_component_test/Lvl_Component_Test_Level.json'
```

The script never pulls, commits, pushes, or runs Perforce commands.

### 4. Build and run AGP

```powershell
Set-Location C:\Development\AGP
.\SetupWindows.bat
```

Follow the FMOD setup prompts. The script also prepares Premake and generates
`Game.sln`. Open the solution in Visual Studio and build `Debug | x64`.
Run `Bin/Debug/Game.exe`.

The build copies game shaders from `Content/Shaders` and then engine shaders
from `Source/Engine/GraphicsEngine/Shaders` into `Bin/Debug/EngineShaders`.
Engine shader sources win when paths overlap. Keep editing the source files;
the deployed `EngineShaders` folder is generated output.

The initial level is selected in `Bin/Settings/ApplicationSettings.json`.
Its scene path is relative to `Content/ExportedScenes/`. Use a scene that exists
in the copied content; machine setup does not require changing the selected level.

### 5. Open the Unreal editor when needed

Open `AGP-Content/Unreal/P5G4_KnightHeart/P5G4_KnightHeart.uproject` with Unreal
Engine 5.7. Its source plugin may need rebuilding on the home PC using the C++
build tools. Unreal generates its local caches and intermediate files.

The optional ExportAndPlay tools use local path files under
`AGP-Content/Tools/Unreal/ExportAndPlay/UE_PythonPaths/`. Run `GenerateFiles.bat`
and follow `SetupGuide.txt` there to set home-machine paths. Those path files
are local and must be regenerated; school paths do not transfer automatically.

## How to work with this setup

### Start a home session

Save existing local changes before updating. Pull both repositories using your
normal Git client, then download their LFS assets. From `C:\Development`:

```powershell
git -C .\AGP lfs pull
git -C .\AGP-Content lfs pull
```

Preview and run `SyncContent.ps1` after content updates. Build AGP if engine code
or shaders changed. Nothing automatically copies content or updates either repo.

### Change engine code or engine shaders

1. Edit files in AGP, including renderer shader sources under `Source/Engine/`.
2. Build and test the engine.
3. Review, commit, and push those changes in AGP.

### Change game assets, game shaders, or levels

1. Edit the Unreal project or shared runtime content in AGP-Content. Game material
   shaders live in `AGP-Content/Content/Shaders/`.
2. Export level JSON/navmesh data through the custom Unreal plugin. Save the
   runtime exports in the appropriate subfolder of
   `AGP-Content/Content/ExportedScenes/`. Keep existing names/folder conventions.
3. If FBX/PNG source assets need preparing, run `ExportAssets.bat` from
   AGP-Content. It copies source FBX files and supported PNGs into runtime
   `Content/`, converts runtime PNGs to DDS, and deletes a runtime PNG only
   after successful conversion. Unreal's source PNGs remain. The original
   keyboard pause is retained, including on errors. This batch file does not
   export Unreal levels or extract files from `.uasset` packages.
4. Preview/run `SyncContent.ps1`, review any overwrite conflicts, then run the
   game. Rebuild AGP when game shader changes need deploying to `EngineShaders`.
5. Review, commit, and push changes in AGP-Content. Include both edited Unreal
   assets and the runtime exports needed by AGP. Commit AGP separately if code
   or engine settings changed as well.

If ExportAndPlay writes a file directly into `AGP/Content/`, preserve the desired
export in AGP-Content too. Otherwise the file remains only in the ignored local
runtime copy. ExportAndPlay does not replace the full content-copy step.

### Before switching computers

Commit and push the relevant changes in both repositories. Files that exist only
on the school PC, uncommitted files, and commits that were not pushed will not
appear on the home PC. This includes the new content scripts and setup docs.

### Back at school: keep the manual workflow

Continue bringing home changes in through Git and reviewing/reconciling offline
work in Perforce as you already do. Check team updates and review the pending
Perforce changelist before submitting.

Perforce supplies the engine's `Content/` through its existing stream import.
The home content-copy script does not replace that mapping or change Perforce's
record of synced revisions. Use it at school only when you deliberately want to
copy local content between the workspaces.

Keep personal Git metadata, this home guide, the Git-oriented README/sync/tests,
and generated `.old` files out of Perforce. The workspace `p4ignore.txt` files
contain the relevant exclusions. Ignore rules do not remove files already
pending for add; review the changelist after reconciling.

## Common issues

- **Content missing after clone/update:** download LFS assets and run
  `SyncContent.ps1`; AGP no longer stores shared content in its own Git history
  going forward.
- **Sync reports differing files:** preserve/review local edits before using
  `-OverwriteChangedFiles`. Do not edit the ignored engine copy as the only copy.
- **LFS pointer detected:** run `git lfs pull` in AGP-Content, then retry.
- **texconv missing:** place `texconv.exe` beside `ExportAssets.bat`. It is only
  required when running the asset conversion step.
- **Wrong/missing scene:** check `initialScene` in the engine's application
  settings and confirm that export exists under `Content/ExportedScenes/`.
- **Old assets remain after sync:** the copy script intentionally retains
  destination-only files; remove obsolete copies manually after review.
