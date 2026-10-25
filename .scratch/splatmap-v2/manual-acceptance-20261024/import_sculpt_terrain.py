"""Replace only the backed-up acceptance terrain asset from the verified sculpt."""
import unreal
import json
from pathlib import Path

root = Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
assert (root / 'before-sculpt/Lvl_Splatmap_Testing2.uasset').exists()
assert not (root / '95-sculpt-mesh-reimport.json').exists()
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
task.filename = str(root / '91-blender-sculpt-corrected.fbx')
task.destination_path = '/Game/Meshes/Landscape/Test'
task.destination_name = 'Lvl_Splatmap_Testing2'
task.automated = True
task.replace_existing = True
task.replace_existing_settings = True
task.save = False
task.options = options
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh = unreal.load_asset('/Game/Meshes/Landscape/Test/Lvl_Splatmap_Testing2')
assert isinstance(mesh, unreal.StaticMesh)
bounds = mesh.get_bounding_box()
report = {'imported_paths': list(task.imported_object_paths), 'source': task.filename,
          'asset': mesh.get_path_name(), 'bounds': str(bounds), 'saved': False,
          'import_data_class': mesh.get_editor_property('asset_import_data').get_class().get_name()}
(root / '95-sculpt-mesh-reimport.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
# Inspect bounds before saving; import alone is not acceptance.
