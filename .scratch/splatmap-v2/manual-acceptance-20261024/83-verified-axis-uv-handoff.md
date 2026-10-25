### Verified axis and UV handoff (2026-10-25 acceptance run)

Export the native Landscape with Unreal **Force Front X Axis** enabled.
The default -Y-front export preserved hills and valleys but rotated the actual
AGP import by 90 degrees relative to native world XY. The verified Blender 5.1.2
handoff exports **X Forward / Z Up**, with `use_space_transform=True`,
`bake_space_transform=True`, baked object transforms and centimetre mesh data.
These settings describe the measured single-mesh fixture; verify a new source
rather than assuming settings alone prove registration.

Blender's standard importer omitted Unreal's `LayerElementBumpUV` WeightmapUVs.
The acceptance correction restores this as explicit UV1 using source vertex
indices. The measured source uses `ByVertice` / `Direct`; reject other mappings
until handled deliberately. Preserve UV0 too, and verify both channels through
AGP's actual importer after the Blender roundtrip. A UV-layer count alone does
not establish unchanged values.

For the current corrected mesh, independent native-height comparison found
zero height error at all 16,384 imported vertices, complete coverage of 16,129
unique grid points, and maximum horizontal rounding 0.000976563 cm. All seven
rotated/reflected alternatives fail. Indexed comparison preserves 95,256 corners
with zero UV0/UV1 differences; face/normal alignment is 0.999565. Corrected FBX
SHA256: `93AB13014248D8891C09575766CED6E2E704123492FFB2D97FE9294ECDADA0C6`.
Evidence and the fixture-specific Blender script are under
`.scratch/splatmap-v2/validation/manual-acceptance-20261024/` (refreshed evidence
snapshots contain `blender_correct_frontx.py` and logs 59..61 and 70).
Do not reuse this mesh/hash for a later sculpt.

These geometry checks do not establish surface parity. Native alpha blending
starts at zero and applies the painted Dunes weight; AGP's specified continuous
Dunes base ignores that base weight. Native mask readback proves their named
contribution formulas differ at 16,335/16,384 component texels, while the 49 fully
painted Cliff texels agree. The current native directional intensity is 10;
preparation deliberately overrides runtime intensity to 2. Matched surface
captures still show substantially different shading. Keep this visual gate and
the sculpt/preservation-fixture gates open until verified and signed off.

The final axis-corrected runtime probe changes 1,851 albedo pixels between
textured and raw RGB modes, with zero changed world-normal, roughness,
metalness or AO pixels. The older 1,865-pixel result below predates the final
axis correction.

