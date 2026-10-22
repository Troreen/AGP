<!-- Managed project context: source is AGP-Content/Docs/ProjectVersionControl.md. -->
# AGP and AGP-Content: version control for agents

Verified on 2026-10-22. Machine paths, branches and client names below are an
observed example, not defaults to force onto other computers. Recheck live state.

## Ownership

| Project/location | Purpose | Version control |
| --- | --- | --- |
| `Troreen/AGP` | C++20/DirectX 11 engine and game application, engine shaders, build files, engine settings | GitHub; also a school Perforce workspace |
| `Troreen/AGP-Content` | Unreal 5.7 project, source assets, game material shaders, exported scenes, runtime FBX/DDS assets, authoring tools | GitHub + Git LFS; also a school Perforce workspace |
| `AGP/Content/` | Local assets consumed by the engine | Ignored by AGP Git; school Perforce import or explicit home copy |
| `AGP/Bin/Settings/` | Application/input/material settings | Tracked in AGP Git, unlike generated `Bin` outputs |
| `AGP/Bin/<configuration>/EngineShaders/` | Deployed shader output | Generated; edit source shaders instead |
| `AGP-Content/Engine/Build/` | Intentionally bundled runtime and libraries for tools | Tracked through LFS for applicable binaries; debug symbols ignored |

GitHub remotes:

- Engine: `https://github.com/Troreen/AGP.git`
- Content: `https://github.com/Troreen/AGP-Content.git`

These are independent repositories. A commit/push in one does not save the
other. Binary LFS contents also need uploading/downloading. GitHub is the
cross-device Git workflow; Perforce is the separate school/team workflow.
There is no automatic bidirectional bridge between them.

## Start every task

1. Read root `AGENTS.md`, the relevant subsystem docs and existing diffs. In AGP,
   read `CODING_STANDARDS.md` before editing code, plus `Docs/AgentWorkflow.md`
   for workflow/verification. Small fixes do not need a full spec/ticket process.
2. Discover both local roots rather than assuming sibling directories. Check
   `git status --short --branch` and `git remote -v` in each affected repository.
   Preserve existing work; do not switch branches or update over it blindly.
3. At school, inspect Perforce with an explicit client, `info`, `where` and
   `opened`. Confirm the client belongs to this machine and maps the intended
   files. Do not change machine-wide `P4CLIENT` merely to make a command work.
4. Identify which repository owns every intended edit and whether the same
   on-disk file is also Perforce-managed. Git and Perforce can overwrite the same
   file independently; their status and history are separate.

## Home and cross-device workflow

Clone both repositories, preferably as siblings named `AGP` and `AGP-Content`.
Install Git LFS, initialize it for the machine, and run `git lfs pull` in each
checkout as applicable. AGP currently retains a legacy LFS attribute for a
texture path under its ignored `Content/`; AGP-Content owns current shared assets.

Save/review local work before updating either branch. Use the user's normal
Git workflow; for a clean matching branch, `git pull --ff-only` avoids creating
an unexpected merge. Inspect divergence rather than resetting it away.

From AGP-Content, preview content deployment (replace the path):

```powershell
.\SyncContent.ps1 -EngineRoot 'C:\Development\AGP' -WhatIf
```

The sibling `../AGP` is the default when `-EngineRoot` is omitted. After reviewing
the preview and preserving any desired destination edits, copy missing files
with the same command without `-WhatIf`. If differing existing files should be
replaced, explicitly add `-OverwriteChangedFiles`.

The script copies runtime `Content/` only, skips identical files, rejects LFS
pointers and retains destination-only files. `-ExcludeRelativePath` preserves a
specific destination file. It does not sync settings, engine code, the bundled
runtime, Git history or Perforce revision records. It does not run automatically.

### Engine changes

Edit in AGP, follow its coding standards, and use its documented build/tests.
The current setup uses Visual Studio 2026/v145 and a Windows SDK; consult AGP's
`README.md`, `SetupWindows.bat` and `Docs/AgentWorkflow.md` for prerequisites.
Do not rerun dependency setup or project generation unnecessarily.

### Asset, level and game-shader changes

1. Edit the owned files in AGP-Content. Game shaders live in `Content/Shaders/`;
   engine shaders live in AGP under `Source/Engine/GraphicsEngine/Shaders/`.
2. Export levels/JSON/navmeshes through the Unreal plugin into the appropriate
   `AGP-Content/Content/ExportedScenes/` folder. Keep established path conventions.
3. Use `ExportAssets.bat` when existing source FBX/PNG files need preparing. It
   requires `texconv.exe`, copies assets and converts supported runtime PNGs to
   DDS; it does not extract `.uasset` packages or export levels. Successful
   conversion deletes the runtime PNG, while original Unreal sources remain.
4. Preview/copy with `SyncContent.ps1`, then build/run AGP as appropriate. The
   engine build deploys content shaders, then engine shaders (engine wins on
   overlapping paths). A content copy alone does not redeploy build shaders.
5. Review changes in both repos. Save Unreal sources and required runtime exports
   in AGP-Content; save engine changes in AGP. Commit/push only when authorized.

An export/edit made only in `AGP/Content/` is not saved by an AGP Git commit.
Preserve desired edits back in AGP-Content and review them before copying again.
Do not force-add assets to AGP as a workaround. Renames/deletions require reviewing
stale runtime copies because deployment deliberately does not mirror deletions.

### Before switching devices

When authorized, commit/push the scoped work in each affected repository. Confirm
LFS upload success, remote branch state and remaining local changes. Record both
commit IDs when an engine change depends on a matching content change. On the
other device, update both branches, download LFS assets, deploy content and build
as needed. Uncommitted/unpushed work and machine-local paths do not transfer.

## School Perforce workflow

The verified school stream setup is:

- Engine stream: `//MAL25-G4/Mainline`.
- Content/Unreal stream: `//MAL25-G4/Content_&_Unreal_Project`.
- Mainline imports `Content/...` from the content stream with an `import`
  mapping. Inspect the current view; do not treat the imported folder as an
  independently writable Mainline content depot or change the mapping.

Observed on this device:

| Purpose | Local root | Perforce client |
| --- | --- | --- |
| Engine | `C:\Users\tarik.bergstrom\Documents\GitHub\AGP` | `tarik.bergstrom_TGA-SP85_Mainline_884` |
| Content | `C:\Users\tarik.bergstrom\Perforce\AGP-Content` | `tarik_content` |

The default client selected in the content directory was the engine client.
Use `p4 -c <verified-client> ...` per command; never infer the client from CWD.
At home these clients/server may not exist. Git-only work does not require
creating or repairing a school Perforce workspace.

Read-only inspection examples (substitute verified client/paths):

```powershell
p4 -c <client> info
p4 -c <client> where '<absolute-file-or-directory>/...'
p4 -c <client> opened
p4 -c <client> reconcile -n '<scoped-path>/...'
```

Before updating either system, preserve local edits and inspect pending changes.
Git pull can modify Perforce-managed files without recording a Perforce edit;
Perforce sync can modify Git-tracked files without updating Git history. Bring
home work into the school workspace through the established manual workflow,
compare team changes, and reconcile only the intended files. Preview first.
Do not run broad forced sync, revert, reconcile or clean operations.

**Perforce submit always requires explicit user authorization for the identified
changelist/scope.** GitHub push permission, successful checks, a workflow skill
or an authenticated login does not grant that permission. Prepare a concrete
file list, diff and validation summary before asking. Never submit through a
wrapper or GUI to evade this rule. If scope changes, obtain matching permission.

Use the correct `p4ignore.txt` separately from `.gitignore`; the rules differ.
Inspect actual ignore results and the pending changelist. Exclusions do not
remove files already tracked or pending for add. Keep Git metadata, personal
agent rules/setup docs and generated files out of team submissions unless the
user explicitly wants them shared with the team. Do not edit server stream
specifications, credentials or other users' files as part of normal development.

## Agent rules and future engine documentation

Shared project-neutral rules are maintained in
`AGP-Content/AgentConfig/GlobalRules.md`. `Docs/AgentSetup.md` there describes
installing them into each device's Codex home and refreshing AGP's local copy
of this guide. AGP's existing coding standard remains authoritative for its code.

For later engine documentation work, keep AGP's root `AGENTS.md` as a short
navigation map. Reuse the current guides for architecture, frame order, framework
interfaces, materials/shaders, input, collision and scripted events. Add small
subsystem guides and nearby `AGENTS.md` entry points where useful; do not duplicate
the whole root guide into every source directory. Each subsystem guide should
cover ownership/lifetimes, execution flow, extension points, invariants, concrete
examples and relevant validation. Link to code and update the guide with API
changes. New documentation structure is a future task, not an installed fact.
