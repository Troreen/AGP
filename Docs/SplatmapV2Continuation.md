# Splatmap V2: home-PC continuation

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
6. Agree the concrete FBX preparation behavior before implementing automation,
   as required by the earlier pipeline discussion. Integrate repeatable transform
   baking into the existing export/preparation route while preserving authored
   FBX, positions, UVs and paint alignment. The current diagnostic baker supports
   only a single root mesh with uniform positive scale; do not treat it as general.
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

`ExportAssets.bat` still mirrors the original unbaked Unreal FBX and can overwrite
the corrected runtime mesh. After any new geometry export, perform a fresh bake
and position/UV/normal verification. Never reuse an old baked candidate after
sculpting. Do not run the full export over the corrected runtime file blindly.

The content checkpoint includes the latest test-map and terrain material-instance
saves for home continuation; their new authoring state has not been re-exported
or validated in AGP. Unrelated village-wall FBX and bundled Engine settings edits
are excluded from this push. Nothing is submitted to Perforce.
