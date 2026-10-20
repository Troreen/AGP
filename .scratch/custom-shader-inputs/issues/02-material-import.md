# 02: Unreal custom parameter binding
Status: in-progress
Blocked by: none
Owner: material-import

Spec: ../spec.md

## Acceptance criteria
- Master/instance scalar and vector names use identical MB_ and space conversion.
- Textures bind by standard asset suffix or exact asset name; exported parameter-name lookup and rename compatibility are excluded (user correction, 2026-10-20).
- Existing export schema preserved; unmatched bindings diagnosed.

## Verification
Use existing material/import public seams where possible; build and verify standard suffix slots, exact asset-name resource binding and scalar/vector name normalization. Record limitations.

## Implementation and evidence

2026-10-20 scope correction: removed exported texture parameter-name lookup and
restored the original suffix-or-asset-name rules. Updated MaterialImportTests to
exercise those rules, including standard suffix priority. The evidence below
records the earlier implementation and does not establish the revised behavior.

Revised verification (2026-10-20): Game.sln Debug x64 built successfully. Updated
MaterialImportTests built with BuildProjectReferences=false against those
libraries and passed on the RTX 4060. The sandboxed test builds stalled and were
cancelled; the test build succeeded outside the sandbox. Linker warnings concerned
missing FMOD debug symbols. No rendered appearance checks were performed.
- Both master/instance imports share private ApplyMaterialParameters; scalar/vector MB_ prefix and space replacement are identical.
- Texture names replace spaces only, use reflected exported-name binding first, then existing standard asset suffix slots, then legacy asset-name lookup. Missing assets and unmatched reflected values/resources include material and parameter diagnostics.
- Public seam: BuildWorld with an actual DirectX 11 device and compiled/reflected MaterialImportFixture.hlsli. The standalone MaterialImportTests project uses pinned assets, avoiding new production test APIs.
- Red: 2026-10-16 baseline build passed, runtime failed `Master scalar parameter with spaces was not imported` after successful material compilation and scene construction.
- Green: `MSBuild Tests/GameFramework/MaterialImportTests.vcxproj /m /nologo /p:Configuration=Debug /p:Platform=x64 /v:minimal /nr:false` passed after implementation.
- `Bin/Tests/Debug/MaterialImportTests.exe` passed on NVIDIA GeForce RTX 4060 with a hidden HWND. Run from repository root with Dependencies/TGAFBXImporter/lib/Debug and Dependencies/FMod/lib on PATH.
- Assertions cover spaced/prefixed scalar/vector names on both master and instance; custom parameter binding despite renamed standard-suffix texture assets; instance override; standard Normal suffix and legacy asset-name fallback inheritance.
- Limitation: these tests verify scene-imported bindings and GPU shader reflection, not rendered pixel appearance. Full integrated standards/spec review is coordinated by root.
