import unreal, builtins, json
from pathlib import Path
root=Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
s=builtins._agp_acceptance_paint
landscape=s['landscape']
rt=unreal.RenderingLibrary.create_render_target2d(landscape,127,127,unreal.TextureRenderTargetFormat.RTF_RGBA8,unreal.LinearColor(0,0,0,0),False,False)
rt.set_editor_property('target_gamma',1.0)
if not landscape.render_weightmap(unreal.Transform(),s['extents'],'Snow_Cliff',rt): raise RuntimeError('Recapture failed')
v=[c.r for c in unreal.RenderingLibrary.read_render_target_raw(landscape,rt,False)]
changed=[i for i,(a,b) in enumerate(zip(s['values'],v)) if a!=b]
outside=[i for i in changed if not(35<=i%127<=41 and 70<=i//127<=76)]
report={'changed_native_samples':len(changed),'outside_patch_samples':len(outside),
        'bounds':[min(i%127 for i in changed),min(i//127 for i in changed),max(i%127 for i in changed),max(i//127 for i in changed)] if changed else None,
        'max':max(v),'examples':[(i%127,i//127,s['values'][i],v[i]) for i in changed[:10]], 'saved':False}
(root/'23-native-paint-verification.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
if outside or len(changed)!=49:
    raise RuntimeError('Native paint differs outside intended patch; map remains unsaved for inspection')
report['saved']=bool(unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
(root/'23-native-paint-verification.json').write_text(json.dumps(report,indent=2))
print('Saved verified paint:',report['saved'])
