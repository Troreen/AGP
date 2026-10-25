import unreal
import json
from pathlib import Path

root = Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
asset_path = '/Game/Meshes/Landscape/Test/Lvl_Splatmap_Testing2'
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    raise RuntimeError('Unexpected existing mesh asset; inspect before replacing')
options = unreal.FbxImportUI()
options.import_mesh = True
options.import_as_skeletal = False
options.import_materials = False
options.import_textures = False
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
options.automated_import_should_detect_type = False
mesh_options = options.static_mesh_import_data
mesh_options.combine_meshes = True
mesh_options.auto_generate_collision = False
mesh_options.generate_lightmap_u_vs = False
mesh_options.transform_vertex_to_absolute = True
mesh_options.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
task = unreal.AssetImportTask()
task.filename = 'C:/Users/tarik/Documents/GitHub/AGP-Content/.scratch/splatmap-v2/validation/manual-acceptance-20261024/fresh-fbx-01/Lvl_Splatmap_Testing2.fbx'
task.destination_path = '/Game/Meshes/Landscape/Test'
task.destination_name = 'Lvl_Splatmap_Testing2'
task.automated = True
task.replace_existing = False
task.save = True
task.options = options
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh = unreal.load_asset(asset_path)
if not isinstance(mesh, unreal.StaticMesh):
    raise RuntimeError('StaticMesh import did not produce the expected asset')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
terrain = next(a for a in actors if a.get_actor_label() == 'Lvl_Splatmap_Testing2')
component = terrain.get_component_by_class(unreal.StaticMeshComponent)
if component.static_mesh:
    raise RuntimeError('Terrain actor no longer has the recorded empty slot')
terrain.modify()
component.modify()
component.set_static_mesh(mesh)
landscape = next(a for a in actors if isinstance(a, unreal.Landscape))
component.set_material(0, landscape.get_editor_property('landscape_material'))
# Keep the native authoring surface visible without overlapping mesh surfaces.
terrain.set_is_temporarily_hidden_in_editor(True)
saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
report = {'imported': list(task.imported_object_paths), 'mesh': mesh.get_path_name(),
          'bounds': str(mesh.get_bounding_box()), 'actor': terrain.get_path_name(),
          'actor_transform': str(terrain.get_actor_transform()),
          'material': component.get_material(0).get_path_name(), 'saved_map': bool(saved),
          'temporary_editor_hidden': True, 'generate_lightmap_uvs': False,
          'normal_import': 'ImportNormalsAndTangents', 'import_materials': False}
(root/'20-fresh-mesh-import.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
if not saved: raise RuntimeError('Map save failed')
