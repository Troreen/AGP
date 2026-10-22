"""Compare final runtime TGAFBX data against original geometry and corrected UVs."""
import json
import math
from pathlib import Path

folder = Path(__file__).resolve().parent / 'fbx-importer-probe'
original = json.loads((folder / 'original-all.json').read_text(encoding='utf-8-sig'))
final = json.loads((folder / 'final-all.json').read_text(encoding='utf-8-sig'))
assert original['totalVertices'] == final['totalVertices'] == 3249
assert original['totalIndices'] == final['totalIndices'] == 18816
errors = {'geometry': 0, 'uv0_formula': 0, 'uv1_formula': 0, 'uv0_v_reflection_removed': 0}
normal_lengths = []
for old_element, new_element in zip(original['elements'], final['elements']):
    assert len(old_element['samples']) == len(new_element['samples']) == 3249
    for old, new in zip(old_element['samples'], new_element['samples']):
        errors['geometry'] = max(errors['geometry'], *(abs(a-b) for a, b in zip(old['position'], new['position'])))
        x, y, z, w = new['position']
        expected0 = [(2800-x)/100, (z+2800)/100]
        expected1 = [(2800-x)/5700, (z+2800)/5700]
        errors['uv0_formula'] = max(errors['uv0_formula'], *(abs(a-b) for a,b in zip(new['uv0'], expected0)))
        errors['uv1_formula'] = max(errors['uv1_formula'], *(abs(a-b) for a,b in zip(new['uv1'], expected1)))
        errors['uv0_v_reflection_removed'] = max(errors['uv0_v_reflection_removed'], abs(new['uv0'][1] - (1-old['uv0'][1])))
        normal_lengths.append(math.sqrt(sum(value*value for value in new['normal'])))
assert max(errors.values()) < 1e-6, errors
assert min(normal_lengths) > 0.999 and max(normal_lengths) < 1.001
result = {'vertices_checked': 3249, 'indices': 18816, 'all_errors_below_1e_minus6': True,
          'maximum_absolute_errors': errors,
          'runtime_uv0_formula': ['(2800-AGP.X)/100', '(AGP.Z+2800)/100'],
          'runtime_uv1_formula': ['(2800-AGP.X)/5700', '(AGP.Z+2800)/5700'],
          'normal_length_range': [min(normal_lengths), max(normal_lengths)],
          'import_yaw_preserved': -90, 'runtime_export_force_front_x_axis': True,
          'note': 'World-projected surface/mask coordinates are independent of mesh UVs.'}
(folder / 'final-verification.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print(json.dumps(result))
