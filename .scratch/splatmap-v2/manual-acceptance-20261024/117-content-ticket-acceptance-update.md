## Manual acceptance update (2026-10-25)

Resume evidence is recorded in engine Docs/SplatmapV2AcceptanceRun-2026-10-24.md,
sections 29 onward, and Content validation/manual-acceptance-20261024 snapshots.
Keep this ticket open; full visual acceptance has not passed.

- Fresh intentional paint and normal TGA re-export pass native allocation,
  unique filenames, exact native RGBA bytes, shared borders and patch checks.
- Intentional sculpt is saved: 69 samples, +200 cm peak, zero outside changes;
  every native weightmap PNG remains byte-identical. Fresh normal export source
  JSON SHA256 53222834068DF11F266C15BD30899FF9E3066CAD7257B024069897881A72FCCC.
- Fresh ForceFrontX raw FBX SHA256
  051E1BBE1BD405C9CAB79E060BF0CF33134ABA2CBFAEF324EEF2A864E43FE86F;
  Blender-corrected runtime FBX SHA256
  29BBC7E277C3ACBA9517763EA39FC7B2D9CFCBA8DFD6522F3D50E455C9DCA72E.
  Actual AGP importer: zero native height error, UV0/UV1 unchanged at 95,256
  corners, normal alignment 0.999512. New source provenance saved in Unreal mesh.
- Conversion, fresh preparation, reviewed five-file deployment and visible
  restarts pass. Game.sln Debug x64/v145: zero warnings/errors. Actual DirectX
  probes pass both scenes; RGB changes 1,851 albedo pixels and zero normals/ORM.
- Separate normal-export fixture: two proxies sharing four native textures,
  eight unique masks and 32,768 native pixel comparisons pass. All 14 original
  ordinary actors and material records preserved; added camera and five custom
  behavior payloads present. Empty custom payload not exercised by chosen actor.
- Matched named-contribution screenshots support matching paint registration,
  including Cliff square versus cubes. Raw RGB and named contributions have
  different red/green meanings. Colors/shading still differ substantially:
  native Dunes is painted alpha, AGP has a continuous base; native light 10,
  runtime override 2. User authorized proceeding with visual failure retained.
  No blend/material contract changed, and final human signoff remains pending.

One editor map-switch crash was caused by a retained Python AssetExportTask
holding the old world (reference chain saved). Saved sculpt map/export/runtime
were intact. Restarted directly on fixture and continued; no force-kill or
source configuration change. No staging, commit, push or Perforce submission.
