import json
from pathlib import Path
import unreal

report = {"assets": [], "actors": []}
root = "/Game/CustomComponents/ScriptedEventComponent"
for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True):
    asset = unreal.load_asset(path)
    entry = {"path": path, "class": asset.get_class().get_name()}
    if isinstance(asset, unreal.Blueprint):
        cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
        cdo = unreal.get_default_object(cls)
        entry["default_object"] = cdo.get_name()
        entry["attributes"] = [name for name in dir(cdo) if not name.startswith("_")]
        for prop in ["Sound", "Positional", "Step", "WaitForCompletion", "TriggerAction", "Replayable", "StringVariables", "IntVariables", "FloatVariables", "BoolVariables"]:
            try:
                entry[prop] = str(cdo.get_editor_property(prop))
            except Exception as error:
                entry[prop] = str(error)
    report["assets"].append(entry)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level.load_level("/Game/Levels/Test_Levels/SP/Lvl_Component_Test"):
    raise RuntimeError("Could not load component-test map")
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    entry = {"name": actor.get_name(), "label": actor.get_actor_label(), "class": actor.get_class().get_name(), "components": []}
    for component in actor.get_components_by_class(unreal.ActorComponent):
        comp = {"name": component.get_name(), "class": component.get_class().get_name()}
        for prop in ["Sound", "Positional", "Step", "WaitForCompletion", "TriggerAction", "Replayable", "TargetActor", "DurationInSeconds", "StringVariables", "IntVariables", "FloatVariables", "BoolVariables"]:
            try:
                comp[prop] = str(component.get_editor_property(prop))
            except Exception:
                pass
        entry["components"].append(comp)
    report["actors"].append(entry)
Path("C:/Users/tarik.bergstrom/Documents/GitHub/AGP/.scratch/scripted-events/unreal-audio-inspection.json").write_text(json.dumps(report, indent=2))
unreal.log("AGP audio inspection complete")
