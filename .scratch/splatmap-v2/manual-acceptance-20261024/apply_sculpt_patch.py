"""Acceptance-only 200 cm mound; verify full packed RG target before import."""
import builtins
import json
from pathlib import Path
import unreal

root = Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
state = builtins._agp_acceptance_geometry
landscape, target = state['landscape'], state['rt']
baseline = json.loads((root / '44-before-sculpt-heights.json').read_text())['heights']
assert state['heights'] == baseline and len(baseline) == 127 * 127
assert not (root / '86-sculpt-import.json').exists(), 'Sculpt already attempted; inspect before repeating'
expected = list(baseline)
for y in range(127):
    for x in range(127):
        radius_squared = (x - 93) ** 2 + (y - 38) ** 2
        if radius_squared < 25:
            expected[y * 127 + x] += round(256 * (1 - radius_squared / 25))
changed = [i for i, (a, b) in enumerate(zip(baseline, expected)) if a != b]
assert len(changed) == 69
canvas, _, context = unreal.RenderingLibrary.begin_draw_canvas_to_render_target(landscape, target)
try:
    # BeginDraw clears the target: reconstruct every packed height, not only the mound.
    for y in range(127):
        for x in range(127):
            height = expected[y * 127 + x]
            canvas.draw_texture(None, unreal.Vector2D(x, y), unreal.Vector2D(1, 1),
                                unreal.Vector2D(0, 0), unreal.Vector2D(1, 1),
                                unreal.LinearColor((height >> 8) / 255, (height & 255) / 255, 0, 1),
                                unreal.BlendMode.BLEND_OPAQUE)
finally:
    unreal.RenderingLibrary.end_draw_canvas_to_render_target(landscape, context)
colors = unreal.RenderingLibrary.read_render_target_raw(landscape, target, False)
actual = [(round(c.r) << 8) | round(c.g) for c in colors]
assert actual == expected, 'Packed target differs; native import was not called'
with unreal.ScopedEditorTransaction('Splatmap V2 acceptance 200cm localized sculpt'):
    landscape.modify()
    imported = landscape.landscape_import_heightmap_from_render_target(target, True, 0)
report = {'imported': bool(imported), 'center_grid': [93, 38], 'center_world_xy_cm': [3000, -2500],
          'radius_cm': 500, 'peak_height_delta_cm': 200, 'changed_target_samples': len(changed),
          'outside_target_changes': 0, 'saved': False, 'pending_native_verification': True,
          'expected_heights': expected}
(root / '86-sculpt-import.json').write_text(json.dumps(report, indent=2))
print(json.dumps({k: v for k, v in report.items() if k != 'expected_heights'}, indent=2))
assert imported, 'Native height import failed'
