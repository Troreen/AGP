import unreal
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
print('WORLD', unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name())
for actor in actors:
    if actor.get_actor_label().startswith('SE_'):
        print('ACTOR', actor.get_actor_label(), actor.get_class().get_path_name(), actor.get_actor_location())
        for c in actor.get_components_by_class(unreal.ActorComponent):
            print('COMPONENT', c.get_name(), c.get_class().get_path_name())
            for p in ['StringVariables','BoolVariables','FloatVariables','IntVariables']:
                try: print(p, c.get_editor_property(p))
                except Exception: pass
print('SUBOBJECT_METHODS', [n for n in dir(unreal.SubobjectDataSubsystem) if any(s in n for s in ['add','delete','gather','rename'])])
print('CUSTOM_ASSETS', unreal.EditorAssetLibrary.list_assets('/Game/CustomComponents', recursive=True))
