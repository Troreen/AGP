import bpy
import json
from pathlib import Path

root = Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
source = Path('C:/Users/tarik/Documents/GitHub/AGP-Content/.scratch/splatmap-v2/validation/manual-acceptance-20261024/fresh-fbx-01/Lvl_Splatmap_Testing2.fbx')
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(source), use_custom_normals=True)
rows = []
for obj in bpy.context.scene.objects:
    row = {'name': obj.name, 'type': obj.type, 'parent': obj.parent.name if obj.parent else None,
           'matrix_world': [list(r) for r in obj.matrix_world], 'dimensions': list(obj.dimensions)}
    if obj.type == 'MESH':
        mesh = obj.data
        row.update(vertices=len(mesh.vertices), polygons=len(mesh.polygons),
                   custom_normals=mesh.has_custom_normals,
                   uv_layers=[l.name for l in mesh.uv_layers],
                   first_positions=[list(v.co) for v in list(mesh.vertices)[:3]],
                   first_normals=[list(n.vector) for n in list(mesh.corner_normals)[:3]])
    rows.append(row)
report = {'blender': bpy.app.version_string, 'source': str(source), 'objects': rows}
(root / '10-blender-inspection.json').write_text(json.dumps(report, indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(root / '10-source-inspection.blend'))
print(json.dumps(report, indent=2))
