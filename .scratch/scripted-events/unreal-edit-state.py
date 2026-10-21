import unreal
ed = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
print('GAME_WORLD', ed.get_game_world())
bp = unreal.load_asset('/Game/CustomComponents/ScriptedEventComponent/BP_SE_WorkflowTest')
for p in ['status', 'blueprint_type', 'parent_class', 'run_construction_script_on_drag', 'is_newly_created', 'bIsReadOnly']:
    try: print('PROPERTY', p, bp.get_editor_property(p))
    except Exception: pass
asset_sub = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
print('ASSET_EDITOR_API', [n for n in dir(asset_sub) if any(k in n for k in ['open','edit','find'])])
print('REFRESH_DOC', unreal.BlueprintEditorLibrary.refresh_open_editors_for_blueprint.__doc__)
print('OPEN_ASSETS', [(a.get_path_name(), a.get_class().get_name()) for a in asset_sub.get_all_edited_assets()])
