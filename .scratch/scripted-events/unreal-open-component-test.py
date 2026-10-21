import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
target = '/Game/Levels/Test_Levels/SP/Lvl_Component_Test'
if world.get_path_name().split('.')[0] != target:
    dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not dirty, 'Current level has unsaved changes; leaving it open: ' + str(dirty)
    assert unreal.EditorLoadingAndSavingUtils.load_map(target)
print('Opened component test level for placed-component verification')
