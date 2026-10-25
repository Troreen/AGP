# 02: Verify the teacher exporter against the painted Landscape

Status: in-progress
Blocked by: none (01 resolved)
Owner: root
Spec: ../spec.md

Build the merged plugin and use Unreal MCP to export
/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing into an isolated validation folder.
Inspect the loaded scene first; preserve existing paint, sculpting and material
assets. Confirm the selected editor runs this project and the rebuilt module.

- [x] Merged plugin compiles for Unreal 5.7 Development Editor.
- [x] Rebuilt DLL, PDB and module manifest are installed; hashes match the build
  output and module BuildId matches installed Unreal 5.7 (47537391).
- [ ] Rebuilt plugin is observed loading successfully in Unreal 5.7.
- [x] Readback safety, filename collision and error propagation findings are
  addressed in source; duplicate weightmap export logic is consolidated.
- [ ] Normal TGA export produces landscape materials and nonempty painted masks.
- [ ] Pixel format, layer/channel mapping, dimensions and naming are verified
  against the actual painted scene, including shared weightmaps/multiple proxies.
- [ ] Geometry export/reimport requirements and runtime component-ID handling
  are recorded from actual output; no prepared-mask workaround is assumed.
- [ ] Cameras, ordinary materials, ActorComponents including empty custom data,
  and skylights retain their expected exported behavior.
- [ ] Export errors and any confirmed defects are fixed or explicitly recorded;
  repeat export produces consistent files without silent collisions.

## Verification

Build and installation completed on 2026-10-23. UnrealBuildTool reported
`Result: Succeeded` for the merged plugin with the safety fixes. Installed DLL,
PDB and UnrealEditor.modules SHA256 hashes match the isolated build output;
BuildId matches installed UE 5.7 (47537391). Local build log is
`../build/build-success.log`; details are in ../workflow.md.

Teacher merge checkpoint 8135cf8 and fixes/build record d27edd8 were pushed to
GitHub on ExporterUpdateBranch with user authorization. No Perforce submission.

The user narrowed the completed task to building the plugin. Editor load,
painted-scene export, channel interpretation and repeat-export behavior remain
unverified; this issue remains open. Unreal MCP was not exposed in that session.

## Review findings addressed in d27edd8

The three source findings have been addressed:

1. Require BGRA8, positive dimensions, a bounded pixel count and an exact bulk
   byte count before copying. Unlock bulk data when a locked read returns null.
2. Include landscape proxy and component object names in mask filenames rather
   than using component indices that restart for each actor.
3. Fail the export on missing/unsupported reads or failed PNG writes. JSON write
   failures also return failure, and the export command displays an error.

The previously unused ExportLandscapeWeightmaps helper now owns the readback
path and returns success/failure; the duplicate inline implementation was removed.
Compilation passed. Actual texture readback and repeated exports still need the
painted-scene checks above.

## Minimal mask repair (2026-10-23)

User approved the minimal repair after reviewing the diagnosis in
`../validation/mask-diagnosis/diagnosis.md`. The new painted scene is
`/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing2`: four 63-quad components,
64x64 masks, no reimported terrain static mesh (12 cube actors remain).

The exporter now reads final weightmap Source mip 0, validates BGRA8 and byte
count, and uses explicit lossless PNG encoding. Packed linear RGBA and existing
filenames are preserved. Landscape component JSON adds `Weightmaps`, each with
`File`, `TextureIndex`, `Width`, `Height`, and `Layers` (`Name`, `Channel`, where
0=R, 1=G, 2=B, 3=A). Allocated layer references/indices/channels are validated
before writing. Geometry, material hierarchy, and runtime support are unchanged.

UE 5.7 Development Editor build passed; log `../build/minimal-repair.log`.
DLL/PDB/manifest installed with hashes matching the isolated build. DLL SHA256:
`8EA43C9FB8C7EB79B7FE29049BF946929887079E33055EA3987F01E4D1CB3D0C`.
Independent spec review: no findings. Independent standards review: no hard
findings; optional filename-format duplication suggestion deferred.

After restart, the user ran normal TGA export into `../validation/export-02`.
Verification script `../validation/mask-diagnosis/Verify-RepairedExport.ps1` passed:
four real 64x64 PNGs, all 16,384 RGBA pixels exactly match the source captures,
and all dimensions, filenames and layer/channel assignments agree. Unique color
counts per component: 896, 1602, 2511, 1749. The source captures also matched
all 20 sampled Unreal paint queries. Export log reports success at 14:02:58 UTC.
Results: `../validation/export-02/verification.json`.

The minimal mask repair is verified. Ticket 02 stays in-progress for its broader
geometry/material, preservation-fixture, multi-proxy and repaint/re-export checks;
ticket 03 runtime acceptance remains blocked. No commit/push/Perforce submit occurred.
