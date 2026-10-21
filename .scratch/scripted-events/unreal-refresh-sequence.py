import unreal
bp = unreal.load_asset('/Game/CustomComponents/ScriptedEventComponent/BP_SE_WorkflowTest')
unreal.BlueprintEditorLibrary.refresh_open_editors_for_blueprint(bp)
print('Refreshed the open BP_SE_WorkflowTest editor; no components removed and no assets saved.')
