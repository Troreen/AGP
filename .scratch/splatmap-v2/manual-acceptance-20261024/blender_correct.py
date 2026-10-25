"""Create a separate terrain FBX candidate in Blender, then inspect its reimport."""
import bpy
import hashlib
import json
from mathutils import Matrix
from pathlib import Path

root = Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
bpy.ops.wm.open_mainfile(filepath=str(root / '10-source-inspection.blend'))
objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
if len(objects) != 1 or objects[0].parent:
    raise RuntimeError('Expected one unparented terrain mesh')
obj = objects[0]
mesh = obj.data
uv_before = [[tuple(uv.uv) for uv in layer.data] for layer in mesh.uv_layers]
positions_before = [tuple(obj.matrix_world @ vertex.co) for vertex in mesh.vertices]
obj.select_set(True)
bpy.context.view_layer.objects.active = obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
# FBX metadata and mesh data both use centimetres; keep the exported node identity.
mesh.transform(Matrix.Scale(100.0, 4))
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 0.01
uv_after = [[tuple(uv.uv) for uv in layer.data] for layer in mesh.uv_layers]
if uv_before != uv_after:
    raise RuntimeError('UVs changed during correction')
position_error = max((vertex.co / 100.0 - __import__('mathutils').Vector(old)).length for vertex, old in zip(mesh.vertices, positions_before))
if position_error > 0.0001:
    raise RuntimeError('World shape changed')
output = root / '11-blender-corrected.fbx'
if output.exists():
    raise RuntimeError('Candidate exists; refusing overwrite')
settings = dict(use_selection=True, object_types={'MESH'}, global_scale=1.0,
                apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL',
                axis_forward='-Z', axis_up='Y', use_space_transform=True,
                bake_space_transform=True, use_mesh_modifiers=False,
                mesh_smooth_type='OFF', add_leaf_bones=False, bake_anim=False,
                use_tspace=True)
bpy.ops.export_scene.fbx(filepath=str(output), **settings)
bpy.ops.wm.save_as_mainfile(filepath=str(root / '11-corrected-terrain.blend'))
report = {'blender': bpy.app.version_string, 'candidate': str(output),
          'sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
          'max_blender_world_position_error_m': position_error,
          'uv_values_unchanged_during_bake': True, 'custom_normals': mesh.has_custom_normals,
          'settings': {k: sorted(v) if isinstance(v, set) else v for k,v in settings.items()}}
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(output), use_custom_normals=True)
report['reimport'] = [{'name': o.name, 'parent': o.parent.name if o.parent else None,
                       'matrix': [list(r) for r in o.matrix_world],
                       'vertices': len(o.data.vertices), 'uv_layers': [l.name for l in o.data.uv_layers],
                       'custom_normals': o.data.has_custom_normals}
                      for o in bpy.context.scene.objects if o.type == 'MESH']
bpy.ops.wm.save_as_mainfile(filepath=str(root / '11-reimport-inspection.blend'))
(root / '11-blender-correction.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
