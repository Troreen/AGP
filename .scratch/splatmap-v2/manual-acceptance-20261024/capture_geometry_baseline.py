import unreal, builtins, json
from pathlib import Path

root=Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
landscape=next(a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a,unreal.Landscape))
rt=unreal.RenderingLibrary.create_render_target2d(landscape,127,127,unreal.TextureRenderTargetFormat.RTF_RGBA8,unreal.LinearColor(0,0,0,0),False,False)
rt.set_editor_property('target_gamma',1.0)
if not landscape.landscape_export_heightmap_to_render_target(rt,True,True):
    raise RuntimeError('Native packed-height export failed')
colors=unreal.RenderingLibrary.read_render_target_raw(landscape,rt,False)
heights=[(round(c.r)<<8)|round(c.g) for c in colors]
print(json.dumps({'sample_count':len(heights),'min':min(heights),'max':max(heights),'zero_samples':heights.count(0),'first_rg':[(c.r,c.g) for c in colors[:8]]}))
if len(heights)!=127*127 or not(30000<min(heights)<max(heights)<40000):
    raise RuntimeError('Packed heights do not match recorded terrain range')
builtins._agp_acceptance_geometry={'landscape':landscape,'heights':heights,'rt':rt}
report={'width':127,'height':127,'min_packed_height':min(heights),'max_packed_height':max(heights),
        'height_cm_range':[(min(heights)-32768)*100/128,(max(heights)-32768)*100/128],
        'heights':heights,'geometry_changed':False}
(root/'44-before-sculpt-heights.json').write_text(json.dumps(report))
print(json.dumps({k:v for k,v in report.items() if k!='heights'},indent=2))
