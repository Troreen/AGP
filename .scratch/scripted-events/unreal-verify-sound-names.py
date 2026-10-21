import unreal
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
base = '/Game/CustomComponents/ScriptedEventComponent/ActionComponents/PlayAudioActionComponent'
default = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(base))
assert 'Sound' not in dict(default.get_editor_property('IntVariables'))
assert dict(default.get_editor_property('StringVariables'))['Sound'] == 'eMainTheme'
print('VERIFIED default: StringVariables.Sound=eMainTheme; no numeric Sound')
bp = unreal.load_asset('/Game/CustomComponents/ScriptedEventComponent/BP_SE_WorkflowTest')
for handle in sub.k2_gather_subobject_data_for_blueprint(bp):
    component = lib.get_object(lib.get_data(handle))
    if component.get_name() == 'SlashSound_GEN_VARIABLE':
        assert 'Sound' not in dict(component.get_editor_property('IntVariables'))
        assert dict(component.get_editor_property('StringVariables'))['Sound'] == 'eSlash'
        print('VERIFIED Blueprint SlashSound: StringVariables.Sound=eSlash; no numeric Sound')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
print('CURRENT_LEVEL', world.get_path_name())
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_actor_label() == 'SE_WorkflowTest':
        for component in actor.get_components_by_class(unreal.ActorComponent):
            if component.get_name() == 'SlashSound':
                print('PLACED_SOUND', dict(component.get_editor_property('StringVariables')), dict(component.get_editor_property('IntVariables')))
