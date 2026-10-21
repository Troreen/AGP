import json
from pathlib import Path

root = Path(__file__).resolve().parents[2]
scene_path = root / 'Content/ExportedScenes/lvl_component_test/Lvl_Component_Test_Level.json'
snapshot_path = Path(__file__).with_name('unreal-sequence-snapshot.json')
text = scene_path.read_bytes().decode('utf-8')
scene = json.loads(text)
actor = next(a for a in scene['Actors'] if a['Name'] == 'SE_WorkflowTest')
records = json.loads(snapshot_path.read_text(encoding='utf-8'))
assert len(records) == 12
# Preserve the existing coordinator, collision shape and all other scene records.
retained = [c for c in actor['Components'] if not c.get('CustomType', '').removesuffix('_C').endswith('ActionComponent')]
assert any(c.get('CustomType') == 'ScriptedEventComponent_C' for c in retained)
assert any(c['Name'] == 'TriggerBox' for c in retained)
new_components = retained + records
actor_start = text.index('"Name": "SE_WorkflowTest"')
components_key = text.index('"Components":', actor_start)
array_start = text.index('[', components_key)
_, length = json.JSONDecoder().raw_decode(text[array_start:])
array_end = array_start + length
newline = '\r\n' if '\r\n' in text else '\n'
replacement = json.dumps(new_components, indent='\t', ensure_ascii=False)
replacement = replacement.replace('\n', newline + '\t\t\t')
new_text = text[:array_start] + replacement + text[array_end:]
new_scene = json.loads(new_text)
expected_scene = json.loads(text)
next(a for a in expected_scene['Actors'] if a['Name'] == 'SE_WorkflowTest')['Components'] = new_components
assert new_scene == expected_scene
scene_path.write_bytes(new_text.encode('utf-8'))
plan_path = Path(__file__).with_name('component-test-sequence.json')
plan = json.loads(plan_path.read_text())
plan['Status'] = 'Applied and saved in Unreal via MCP; live action records synchronized into AGP exported scene'
plan_path.write_text(json.dumps(plan, indent=2) + '\n', encoding='utf-8')
print('Synchronized 12 live Unreal actions; all other scene data preserved.')
