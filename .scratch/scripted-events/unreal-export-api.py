import unreal
for x in unreal.ObjectIterator(unreal.EditorUtilityWidgetBlueprint):
    print('UTILITY_WIDGET', x.get_path_name())
for x in unreal.ObjectIterator(unreal.EditorUtilityBlueprint):
    print('UTILITY_BP', x.get_path_name())
for root in ['/Game/Editor', '/Game/Utilities', '/Game/Blueprints']:
    if unreal.EditorAssetLibrary.does_directory_exist(root):
        print('EDITOR_ASSETS', root, [p for p in unreal.EditorAssetLibrary.list_assets(root, recursive=True) if any(n in p.lower() for n in ['export', 'level'])])
print('EXPORT_SETTINGS', unreal.TGAExportSettingsComponent.__doc__)
print('SERIALIZABLE', unreal.TGASerializableInterface.__doc__)
