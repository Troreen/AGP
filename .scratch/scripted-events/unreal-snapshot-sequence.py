import json
import unreal
from pathlib import Path

root = Path('C:/Users/tarik.bergstrom/Documents/GitHub/AGP')
sequence = next(a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if a.get_actor_label() == 'SE_WorkflowTest')
records = []
for component in sequence.get_components_by_class(unreal.ActorComponent):
    if '/ActionComponents/' not in component.get_class().get_path_name():
        continue
    custom = {}
    for prop in ['StringVariables','BoolVariables','IntVariables','FloatVariables']:
        custom.update(dict(component.get_editor_property(prop)))
    records.append({'Name': component.get_name(), 'Parent': '', 'TypeID': -1,
                    'CustomType': component.get_class().get_name(), 'Tags': [],
                    'Transform': [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1], 'Custom': custom})
assert len(records) == 12
snapshot = root / '.scratch/scripted-events/unreal-sequence-snapshot.json'
snapshot.write_text(json.dumps(records, indent=2) + '\n', encoding='utf-8')
print('Captured', len(records), 'live Unreal action records to', str(snapshot))
