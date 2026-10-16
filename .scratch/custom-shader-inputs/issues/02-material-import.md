# 02: Unreal custom parameter binding
Status: in-progress
Blocked by: none
Owner: material-import

Spec: ../spec.md

## Acceptance criteria
- Master/instance scalar and vector names use identical MB_ and space conversion.
- Custom texture binds by exported parameter name; standard suffix and legacy asset-name fallback retained.
- Existing export schema preserved; unmatched bindings diagnosed.

## Verification
Use existing material/import public seams where possible; build and verify renamed texture asset still binds to declared shader resource. Record limitations.

## Implementation and evidence
- Both master/instance imports share private ApplyMaterialParameters; scalar/vector MB_ prefix and space replacement are identical.
- Texture names replace spaces only, use reflected exported-name binding first, then existing standard asset suffix slots, then legacy asset-name lookup. Missing assets and unmatched reflected values/resources include material and parameter diagnostics.
- Public seam: BuildWorld with an actual DirectX 11 device and compiled/reflected MaterialImportFixture.hlsli. The standalone MaterialImportTests project uses pinned assets, avoiding new production test APIs.
- Red: 2026-10-16 baseline build passed, runtime failed `Master scalar parameter with spaces was not imported` after successful material compilation and scene construction.
- Green: `MSBuild Tests/GameFramework/MaterialImportTests.vcxproj /m /nologo /p:Configuration=Debug /p:Platform=x64 /v:minimal /nr:false` passed after implementation.
- `Bin/Tests/Debug/MaterialImportTests.exe` passed on NVIDIA GeForce RTX 4060 with a hidden HWND. Run from repository root with Dependencies/TGAFBXImporter/lib/Debug and Dependencies/FMod/lib on PATH.
- Assertions cover spaced/prefixed scalar/vector names on both master and instance; custom parameter binding despite renamed standard-suffix texture assets; instance override; standard Normal suffix and legacy asset-name fallback inheritance.
- Limitation: these tests verify scene-imported bindings and GPU shader reflection, not rendered pixel appearance. Full integrated standards/spec review is coordinated by root.
