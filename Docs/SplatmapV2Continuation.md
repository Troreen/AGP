# Splatmap V2: home-PC continuation

For the final hands-on workflow and signoff record, see
[Splatmap V2 manual acceptance](SplatmapV2ManualAcceptance.md).

Saved 2026-10-23. Feature remains in progress. The user authorized saving this
plan and pushing both Git repositories. No Perforce submission is authorized.

## Resume context

- Engine branch: `SplatmapV2`; prior milestone `cb5832a`.
- Content branch: `ExporterUpdateBranch`; prior milestone `bdc8781`.
- Find both local roots on the home PC; do not reuse school absolute paths.
- Authoritative feature records are in AGP-Content `.scratch/splatmap-v2/`:
  `workflow.md`, `spec.md`, issues 02/03, and the validation folders.
- Read both repositories' AGENTS.md and version-control guidance first.
- Update both branches, download Git LFS assets, preview `SyncContent.ps1`,
  review differing destination files, then deploy content and build AGP.
- The local AGP scratch probes/caches are not transferred. Reconstruct probes
  from the committed content validation sources using their documented paths.
- Select the prepared runtime RGB scene explicitly if home startup settings differ.

## Agreed next work

1. Make `MM_Landscape.hlsli` easier to read without changing rendering behavior.
   Use `Layer1` for the current Dunes base, `Layer2` for the Rough overlay, and
   `Layer3` for the final Cliff overlay. Use resources such as `Layer1BaseColor`,
   `Layer1Normal`, `Layer1ORM`, and selectors such as `Layer2MaskChannels`.
   These numbers identify blend order, not packed RGB channel order.
2. Separate coordinate mapping, paint-weight sampling, blend-contribution
   calculation, surface sampling/blending, and debug output into clear steps.
   Retain the normal-projection helper; avoid a broad shader framework refactor.
3. Update preparation parameter names alongside shader resources. Keep actual
   snow asset/layer names in an explicit preparation mapping rather than shader
   logic. The current script also hardcodes Snow_Rough/Dunes/Cliff and their
   texture names; renaming shader locals alone does not make the pipeline generic.
   AGP's SceneWorldBuilder already tries texture parameter roles before suffix
   and asset-name fallbacks. Material naming documentation is stale and needs
   updating to reflect that existing behavior.
4. Preserve the current ordered alpha blend: Layer1 is the continuous base,
   Layer2 overlays it, then Layer3 overlays both. Do not silently switch to
   normalized RGB blending. The base channel is not directly used as a weight.
5. Preserve lit RGB mode: DEBUG=2 replaces only base color, with layer normals
   and ORM active. Keep DEBUG=0 as the textured comparison.
6. Use the accepted manual graphics-team handoff for an engine-ready FBX after
   geometry export. The team chooses its correction tools. Preserve the authoring
   FBX and verify shape, placement, positions, UV registration, mask coverage,
   normals and face/normal alignment before deployment. The observed manual
   roundtrip satisfies this route; no new baking automation is required.
7. Validate a fresh repaint/export/prepare/deploy/restart cycle, then a geometry
   change/export/bake cycle. Compare masks, borders, orientation, normals, ORM
   and textured appearance against Unreal. Keep the feature open until observed.

## Verified baseline and remaining gap

On 2026-10-23, the preparation regression suite passed again using a workspace
temporary directory. The existing actual DirectX RuntimePreparationProbe passed:
15 actors, two master materials, 16,129 mesh vertices, face/normal alignment
0.999565, and ten surface/noise texture bindings. RGB mode changed 1,865 albedo
pixels and zero world-normal, AO, roughness or metallic pixels. This reran the
existing probe executable; it was not a fresh engine/probe build.

Source/deployed shader, corrected runtime FBX, RGB scene and combined mask hashes
match. No shader cleanup has been implemented yet, and no fresh Unreal export or
visual parity check was performed in this analysis session.

`ExportAssets.bat` still mirrors the original Unreal FBX and can overwrite the
corrected runtime mesh. After any new geometry export, make the manual correction
the final FBX step before deployment, with position/UV/normal verification. Never
reuse an old corrected mesh after sculpting.

The content checkpoint includes the latest test-map and terrain material-instance
saves for home continuation; their new authoring state has not been re-exported
or validated in AGP. Unrelated village-wall FBX and bundled Engine settings edits
are excluded from this push. Nothing is submitted to Perforce.

## 2026-10-24 focused cleanup and verification

The Landscape shader and preparation tools now use numbered blend roles:
Layer1=Snow_Dunes base, Layer2=Snow_Rough overlay, Layer3=Snow_Cliff final
overlay. The blend math, 8 m repeat, projected normals, debug modes and actual
snow asset names remain. Both current prepared scenes and three earlier terrain
comparison scenes have matching parameter roles. The accepted FBX handoff is
manual; the graphics team chooses its tools and verifies an engine-ready mesh
after export, before deployment.

The painted-landscape regression suite passed. All four shader entry points
compiled with fxc /WX. A fresh v145 DirectX probe against the scoped deployed
runtime passed for both textured DEBUG=0 and lit RGB DEBUG=2 scenes: 15 actors,
16,129 terrain vertices, face/normal alignment 0.999565, 10 surface/noise
bindings, and 8 m surface mapping. RGB changed 1,865 albedo pixels and zero
world-normal, AO, roughness or metallic pixels. This is a runtime binding and
render-output check, not a new Unreal export or a visual parity judgment.

Fresh repaint/export/prepare/deploy/restart, new geometry/export/manual
correction/deploy, and final visual comparison with Unreal remain required.

## Remaining acceptance and Blender handoff

The graphics team uses Blender for the manual FBX correction. Follow the owned
content guide, `AGP-Content/Tools/Terrain/PreparePaintedLandscapeRuntime.md`, for
the ordered handoff and checks. Keep the newly exported Unreal FBX as the source;
the corrected FBX is a separate runtime deliverable. `ExportAssets.bat` mirrors
the source FBX, so place the Blender export at the runtime mesh path after that
batch step and before preparing/deploying content. Record both hashes in the
preparation provenance; a hash from before the replacement is stale.

To close the feature, observe a new paint change in Unreal, run the normal TGA
export, verify current layer/channel metadata and component borders, prepare both
runtime scenes, preview/copy content and restart AGP. Then observe a new sculpt,
export its new geometry, correct that FBX in Blender and verify its positions,
UVs, mask registration and imported normals in AGP. Compare the latest textured
and lit RGB scenes to Unreal at the same landmarks, including corners, seams,
orientation, normals, ORM and surface appearance. Historical hashes and probe
results establish the previous baseline only. Ticket 02 also retains broader
multi-proxy and non-landscape preservation checks recorded in its checklist.
