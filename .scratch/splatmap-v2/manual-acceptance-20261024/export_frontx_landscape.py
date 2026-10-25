"""Run inside Unreal through MCP; export selected native terrain without saves."""
import hashlib
import json
from pathlib import Path
import unreal


world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if world.get_path_name() != "/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing2.Lvl_Splatmap_Testing2":
    raise RuntimeError("Unexpected map; refusing export")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
landscapes = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.LandscapeProxy)]
if len(landscapes) != 1:
    raise RuntimeError("Expected exactly one native Landscape")
output = Path("C:/Users/tarik/Documents/GitHub/AGP-Content/.scratch/splatmap-v2/validation/manual-acceptance-20261024/fresh-fbx-frontx-01")
if output.exists():
    raise RuntimeError("Evidence folder already exists; refusing overwrite")
output.mkdir(parents=True)
fbx = output / "Lvl_Splatmap_Testing2.fbx"
options = unreal.FbxExportOption()
settings = {
    "ascii": False,
    "force_front_x_axis": True,
    "collision": False,
    "level_of_detail": False,
    "vertex_color": True,
    "fbx_export_compatibility": unreal.FbxExportCompatibility.FBX_2013,
}
for name, value in settings.items():
    options.set_editor_property(name, value)
task = unreal.AssetExportTask()
task.object = world
task.filename = str(fbx)
task.automated = True
task.prompt = False
task.selected = True
task.replace_identical = False
task.exporter = unreal.LevelExporterFBX()
task.options = options
previous_selection = actors.get_selected_level_actors()
try:
    actors.set_selected_level_actors(landscapes)
    succeeded = unreal.Exporter.run_asset_export_task(task)
finally:
    actors.set_selected_level_actors(previous_selection)
report = {
    "success": bool(succeeded),
    "errors": list(task.errors),
    "engine": unreal.SystemLibrary.get_engine_version(),
    "world": world.get_path_name(),
    "selected_actor": landscapes[0].get_path_name(),
    "actor_transform": str(landscapes[0].get_actor_transform()),
    "exporter": "LevelExporterFBX",
    "settings": {name: str(options.get_editor_property(name)) for name in settings},
    "fbx": str(fbx),
    "bytes": fbx.stat().st_size if fbx.exists() else 0,
    "sha256": hashlib.sha256(fbx.read_bytes()).hexdigest() if fbx.exists() else None,
    "map_saved": False,
    "paint_or_geometry_changed": False,
}
(output / "export.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps(report, indent=2))
if not succeeded or report["bytes"] == 0:
    raise RuntimeError("Fresh Landscape FBX export failed")
