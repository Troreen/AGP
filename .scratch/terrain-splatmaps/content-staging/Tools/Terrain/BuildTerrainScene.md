# Prepare the separate terrain verification scene

Run `BuildTerrainScene.py` with Python 3. It uses only the standard library.
Run it after exporting the corrected terrain and preparing linear mask/noise
DDS assets. The original test scene, Unreal map and application settings are
not changed.

From the content repository root:

```powershell
python Tools/Terrain/BuildTerrainScene.py --geometry-bounds-json <imported-bounds.json>
```

The default input is
`Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json`.
The output is the new
`Content/ExportedScenes/lvl_splatmap_runtime/Lvl_Splatmap_Runtime_Level.json`
and a `.provenance.json` sidecar. `--content-root`, `--source` and `--output`
override locations. An existing output requires explicit `--force`; the source
scene cannot be used as the output.

`--noise-strength` accepts 0 through 1, default 0. `--noise-patch-size` is in
centimetres, minimum 1, default 200. `--bounds MINX MINY MAXX MAXY` describes
the mask rectangle in Unreal world XY centimetres, default
`-2800 -2800 2800 2800`. `--surface-origin X Y` defaults to `-2800 -2800`;
surface textures repeat every 800 centimetres independently of mask coverage.

`--debug-mode` selects 0 for the lit surface, 1 for effective Dunes/Rough/Cliff
contributions, 2 for raw splat channels, 3 for encoded world normal or 4 for ORM.
The default is 0. Restart the game after changing scene material parameters;
the preparation tool does not establish whether a running game reloads them.

The prepared scene folder also includes two comparison scenes:

- `Lvl_Splatmap_Runtime_Noise_Level.json`: lit surface, noise strength 0.75.
- `Lvl_Splatmap_Runtime_Weights_Level.json`: effective contribution colors,
  noise strength 0.

Each has its own provenance sidecar and the same validated geometry, camera,
mapping and surface repeat as the baseline scene. Generate a new variant with
`--output <new-scene-path> --noise-strength 0.75 --debug-mode 0`, or with
`--output <new-scene-path> --noise-strength 0 --debug-mode 1` for contributions.

The required bounds input must come from the **runtime FBX importer** after
the final asset export, in imported **local AGP XYZ** coordinates:

```json
{
  "coordinate_system": "AGP",
  "meshes": {
    "SM_Landscape_Splatmap": {"min": [-2800, 0, -2800], "max": [2800, 24.21875, 2800]},
    "SM_Plane": {"min": [-100, 0, -100], "max": [100, 0, 100]}
  }
}
```

Do not infer those bounds from actor positions or copy these example numbers
for a changed mesh. The tool transforms all eight AABB corners through the
component and actor transforms using the same axis permutation as
`UnrealSceneImporter`. Every static mesh must have bounds and fit entirely
inside the mask rectangle. Missing bounds or geometry outside coverage abort
before output creation. The sidecar records input hashes, mapping controls
and each checked world AABB.

The scene uses `SM_Landscape_Splatmap` from
`Content/Meshes/Landscapes/Splatmap/SM_Landscape_Splatmap.fbx`, all nine
`T_8M_Snow_*` textures, `T_Splatmap_LevelWeights` and
`T_Splatmap_TransitionNoise`. The 800-centimetre plane fixtures sit at Unreal
X positions -1000 and +1000, height 80, with yaw 0 and 45 degrees. They are
elevated inspection surfaces. They do not establish matching terrain height
or visual correctness. The scene includes an oblique camera at Unreal
`(0,-3500,3200)` looking toward the origin, with a 90-degree horizontal field
of view. Its component uses the actual importer key `FOV`. The runtime visual
checks still need appropriate camera framing, contact comparison, debug views,
noise adjustment and captures.

Coverage validation does not establish image registration, image channel
meaning, DDS conversion correctness or final appearance. Preserve the terrain's
intentional import rotation; this tool adds no corrective terrain rotation.
