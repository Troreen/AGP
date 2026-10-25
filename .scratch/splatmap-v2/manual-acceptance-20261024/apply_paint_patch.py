import unreal, builtins, json
from pathlib import Path
root = Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
state = builtins._agp_acceptance_paint
landscape, rt = state['landscape'], state['rt']
canvas, size, context = unreal.RenderingLibrary.begin_draw_canvas_to_render_target(landscape, rt)
for y in range(127):
    x = 0
    while x < 127:
        value = state['values'][y * 127 + x]
        end = x + 1
        while end < 127 and state['values'][y * 127 + end] == value:
            end += 1
        color = value / 255.0
        canvas.draw_texture(None, unreal.Vector2D(x,y), unreal.Vector2D(end-x,1), unreal.Vector2D(0,0),
                            unreal.Vector2D(1,1), unreal.LinearColor(color,color,color,1), unreal.BlendMode.BLEND_OPAQUE)
        x = end
canvas.draw_texture(None, unreal.Vector2D(35,70), unreal.Vector2D(7,7), unreal.Vector2D(0,0),
                    unreal.Vector2D(1,1), unreal.LinearColor(1,1,1,1), unreal.BlendMode.BLEND_OPAQUE)
unreal.RenderingLibrary.end_draw_canvas_to_render_target(landscape, context)
values = [c.r for c in unreal.RenderingLibrary.read_render_target_raw(landscape,rt,False)]
changed = [i for i,(a,b) in enumerate(zip(state['values'],values)) if a != b]
outside = [i for i in changed if not (35 <= i % 127 <= 41 and 70 <= i // 127 <= 76)]
if outside or len(changed) != 49:
    raise RuntimeError('Transient paint target differs outside the exact 7x7 patch')
with unreal.ScopedEditorTransaction('Splatmap V2 acceptance Snow_Cliff patch'):
    landscape.modify()
    imported = landscape.landscape_import_weightmap_from_render_target(rt,'Snow_Cliff',0)
report={'layer':'Snow_Cliff','changed_target_pixels':len(changed),'outside_target_changes':len(outside),
        'imported':bool(imported),'saved':False,'pending_native_weight_verification':True}
(root/'22-paint-import.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
if not imported: raise RuntimeError('Landscape rejected paint import')
