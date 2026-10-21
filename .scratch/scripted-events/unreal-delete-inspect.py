import unreal
bp = unreal.load_asset('/Game/CustomComponents/ScriptedEventComponent/BP_SE_WorkflowTest')
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
print('DATA METHODS', [n for n in dir(lib) if any(k in n for k in ['name','delete','blueprint','variable'])])
print('BP METHODS', [n for n in dir(unreal.BlueprintEditorLibrary) if any(k in n for k in ['variable','structur','refresh'])])
for h in sub.k2_gather_subobject_data_for_blueprint(bp):
    d = lib.get_data(h)
    o = lib.get_object(d)
    if 'ObserveResult' in o.get_name():
        print('OBJECT', o.get_path_name(), 'FLAGS', lib.can_delete(d), lib.can_edit(d))
        for n in ['get_variable_name','get_display_name','get_blueprint','get_object_for_blueprint']:
            if hasattr(lib,n):
                try: print(n, getattr(lib,n)(d))
                except Exception as e: print(n, str(e))
print('RENAME MEMBER DOC', sub.rename_subobject_member_variable.__doc__)
print('RENAME DOC', sub.rename_subobject.__doc__)
