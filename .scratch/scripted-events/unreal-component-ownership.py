import unreal
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_selected_level_actors()
print('SELECTED', [(a.get_actor_label(), a.get_class().get_path_name()) for a in actors])
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
print('FLAGS', [n for n in dir(lib) if any(k in n for k in ['delete','inherited','native','instance','edit'])])
bp = unreal.load_asset('/Game/CustomComponents/ScriptedEventComponent/BP_SE_WorkflowTest')
for h in sub.k2_gather_subobject_data_for_blueprint(bp):
    data = lib.get_data(h)
    obj = lib.get_object(data)
    flags = {}
    for name in ['can_delete', 'can_edit', 'is_inherited_component', 'is_native_component', 'is_instanced_component']:
        if hasattr(lib, name):
            flags[name] = getattr(lib, name)(data)
    print('BLUEPRINT', obj.get_name(), flags)
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == 'SE_WorkflowTest':
        for c in a.get_components_by_class(unreal.ActorComponent):
            print('INSTANCE', c.get_name(), c.get_editor_property('creation_method'))
