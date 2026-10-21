import unreal
bp = unreal.load_asset('/Game/CustomComponents/ScriptedEventComponent/BP_SE_WorkflowTest')
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = sub.k2_gather_subobject_data_for_blueprint(bp)
for h in handles:
    d = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(d)
    print('TEMPLATE', obj.get_name(), obj.get_class().get_path_name())
print('ADD', unreal.SubobjectDataSubsystem.add_new_subobject.__doc__)
print('DELETE', unreal.SubobjectDataSubsystem.delete_subobjects.__doc__)
print('RENAME', unreal.SubobjectDataSubsystem.rename_subobject.__doc__)
print('PARAMS', unreal.AddNewSubobjectParams.__doc__)
print('FUNCTIONS', [n for n in dir(unreal) if any(s in n.lower() for s in ['export','tga'])])
print('DATA_LIB', [n for n in dir(unreal.SubobjectDataBlueprintFunctionLibrary) if any(s in n for s in ['object','root','parent'])])
