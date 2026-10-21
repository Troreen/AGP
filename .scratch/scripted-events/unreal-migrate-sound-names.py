import json
import re
import shutil
from pathlib import Path
import unreal

root = Path('C:/Users/tarik.bergstrom/Documents/GitHub/AGP')
header = (root / 'Source/Engine/GameFramework/AudioManager.h').read_text()
body = re.search(r'enum SoundID\s*\{(.*?)\};', header, re.S).group(1)
names = re.findall(r'\b(e\w+)\b', body)
names.remove('eSoundCount')
backup = root / '.scratch/scripted-events/before-sound-names'
backup.mkdir(exist_ok=True)
project_content = Path(unreal.Paths.project_content_dir())
base_path = '/Game/CustomComponents/ScriptedEventComponent/ActionComponents/PlayAudioActionComponent'
audio_bp = unreal.load_asset(base_path)
audio_class = unreal.EditorAssetLibrary.load_blueprint_class(base_path)
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary

def convert(component):
    ints = dict(component.get_editor_property('IntVariables'))
    strings = dict(component.get_editor_property('StringVariables'))
    if 'Sound' not in ints and 'Sound' in strings:
        assert strings['Sound'] in names, strings['Sound']
        return False
    sound = ints.pop('Sound', 0)
    assert 0 <= sound < len(names), sound
    strings['Sound'] = names[sound]
    component.modify()
    component.set_editor_property('IntVariables', ints)
    component.set_editor_property('StringVariables', strings)
    print('MIGRATED', component.get_path_name(), strings['Sound'])
    return True

blueprints = [audio_bp]
for path in unreal.EditorAssetLibrary.list_assets('/Game/CustomComponents/ScriptedEventComponent', recursive=True):
    asset = unreal.load_asset(path)
    if isinstance(asset, unreal.Blueprint) and asset != audio_bp:
        blueprints.append(asset)

# Capture each authored value before changing the base component's defaults.
targets = []
for bp in blueprints:
    components = []
    if bp == audio_bp:
        components = [unreal.get_default_object(audio_class)]
    else:
        for handle in sub.k2_gather_subobject_data_for_blueprint(bp):
            component = lib.get_object(lib.get_data(handle))
            if component.get_class() == audio_class:
                components.append(component)
    if components:
        package = bp.get_path_name().split('.')[0]
        source = project_content / (package.removeprefix('/Game/') + '.uasset')
        backup_file = backup / (bp.get_name() + '.uasset')
        if not backup_file.exists():
            shutil.copy2(source, backup_file)
        for component in components:
            targets.append((bp, component))

modified = []
for bp, component in targets:
    if convert(component) and bp not in modified:
        bp.modify()
        modified.append(bp)
for bp in modified:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    unreal.BlueprintEditorLibrary.refresh_open_editors_for_blueprint(bp)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
level_modified = False
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for component in actor.get_components_by_class(unreal.ActorComponent):
        if component.get_class().get_name() == 'PlayAudioActionComponent_C':
            level_modified |= convert(component)
            assert 'Sound' not in dict(component.get_editor_property('IntVariables'))
            print('VERIFIED', actor.get_actor_label(), component.get_name(), dict(component.get_editor_property('StringVariables'))['Sound'])
if level_modified or modified:
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, world.get_path_name().split('.')[0])
print('Saved sound-name defaults, Blueprint templates and current level. Numeric Sound entries removed.')
