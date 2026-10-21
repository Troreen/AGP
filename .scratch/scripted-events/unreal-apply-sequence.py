import json
import unreal
from pathlib import Path

root = Path('C:/Users/tarik.bergstrom/Documents/GitHub/AGP')
plan = json.loads((root / '.scratch/scripted-events/component-test-sequence.json').read_text())
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name() == '/Game/Levels/Test_Levels/SP/Lvl_Component_Test.Lvl_Component_Test', world.get_path_name()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
sequence = next(a for a in actors if a.get_actor_label() == plan['SequenceActor'])
bp = unreal.load_asset('/Game/CustomComponents/ScriptedEventComponent/BP_SE_WorkflowTest')
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
handles = sub.k2_gather_subobject_data_for_blueprint(bp)
actor_handle = next(h for h in handles if lib.is_root_actor(lib.get_data(h)))
old_actions = []
for h in handles:
    obj = lib.get_object(lib.get_data(h))
    if '/ActionComponents/' in obj.get_class().get_path_name():
        old_actions.append(h)
classes = {}
for action in plan['Actions']:
    name = action['CustomType']
    classes[name] = unreal.EditorAssetLibrary.load_blueprint_class('/Game/CustomComponents/ScriptedEventComponent/ActionComponents/' + name)
    assert classes[name], name
bp.modify()
deleted = sub.delete_subobjects(actor_handle, old_actions, bp)
assert deleted == len(old_actions), (deleted, len(old_actions))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
for action in plan['Actions']:
    handles = sub.k2_gather_subobject_data_for_blueprint(bp)
    actor_handle = next(h for h in handles if lib.is_root_actor(lib.get_data(h)))
    params = unreal.AddNewSubobjectParams(parent_handle=actor_handle, new_class=classes[action['CustomType']], blueprint_context=bp)
    handle, reason = sub.add_new_subobject(params)
    assert not str(reason), str(reason)
    assert sub.rename_subobject(handle, unreal.Text(action['Name'])), action['Name']
    obj = lib.get_object(lib.get_data(handle))
    custom = {'TargetActor': '', 'TargetComponent': '', 'WaitForCompletion': True}
    custom.update(action['Custom'])
    maps = {'StringVariables': {}, 'BoolVariables': {}, 'IntVariables': {}, 'FloatVariables': {}}
    for key, value in custom.items():
        prop = {str:'StringVariables', bool:'BoolVariables', int:'IntVariables', float:'FloatVariables'}[type(value)]
        maps[prop][key] = value
    obj.modify()
    for prop, values in maps.items():
        obj.set_editor_property(prop, values)
    print('CREATED', action['Name'], json.dumps(custom, sort_keys=True))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
# Blueprint compilation reinstantiates the placed Actor. Reacquire it before saving.
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
sequence = next(a for a in actors if a.get_actor_label() == plan['SequenceActor'])
components = sequence.get_components_by_class(unreal.ActorComponent)
actions = [c for c in components if '/ActionComponents/' in c.get_class().get_path_name()]
assert len(actions) == len(plan['Actions']), [(c.get_name(), c.get_class().get_name()) for c in actions]
expected = {a['Name']:a for a in plan['Actions']}
for c in actions:
    assert c.get_name() in expected, c.get_name()
    actual = {}
    for prop in ['StringVariables','BoolVariables','IntVariables','FloatVariables']:
        actual.update(dict(c.get_editor_property(prop)))
    for key, value in expected[c.get_name()]['Custom'].items():
        assert actual[key] == value, (c.get_name(), key, actual[key], value)
    print('VERIFIED', c.get_name())
coordinator = next(c for c in components if c.get_name() == plan['Coordinator'])
assert dict(coordinator.get_editor_property('StringVariables'))['TriggerCollider'] == 'TriggerBox'
assert any(c.get_name() == 'TriggerBox' for c in components)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Levels/Test_Levels/SP/Lvl_Component_Test')
print('SAVED Blueprint and component test level; verified', len(actions), 'actions')
