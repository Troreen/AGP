import unreal, builtins, json
from pathlib import Path

root = Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
landscape = next(a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a, unreal.Landscape))
rt = unreal.RenderingLibrary.create_render_target2d(landscape, 127, 127, unreal.TextureRenderTargetFormat.RTF_RGBA8, unreal.LinearColor(0,0,0,0), False, False)
rt.set_editor_property('target_gamma', 1.0)
extents = unreal.Box2D(unreal.Vector2D(-6350,-6350), unreal.Vector2D(6350,6350))
if not landscape.render_weightmap(unreal.Transform(), extents, 'Snow_Cliff', rt):
    raise RuntimeError('Weight capture failed')
samples = unreal.RenderingLibrary.read_render_target_raw(landscape, rt, False)
values = [c.r for c in samples]
if len(values) != 127*127 or max(values) <= min(values):
    raise RuntimeError('Unexpected or empty weight capture')
builtins._agp_acceptance_paint = {'landscape': landscape, 'rt': rt, 'extents': extents, 'values': values}
report = {'layer':'Snow_Cliff', 'width':127, 'height':127, 'extents':[-6350,-6350,6350,6350],
          'min':min(values), 'max':max(values), 'values':values,
          'planned_patch_pixels':[35,70,41,76], 'planned_world_xy_cm':[-2800,700,-2200,1300],
          'nearby_landmark':'SM_Cube4 at (-2540,-890,150); patch is north of it'}
(root/'21-before-paint-weights.json').write_text(json.dumps(report))
print(json.dumps({k:v for k,v in report.items() if k != 'values'},indent=2))
