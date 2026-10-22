"""Create and verify the authorized disposable UV-convention test asset."""

import array
import hashlib
import json
from pathlib import Path
import runpy
import sys


scratch = Path(__file__).resolve().parent
source = Path.home() / 'Perforce/AGP-Content/Content/Meshes/Landscapes/Test/SM_Landscape_Test.fbx'
destination = scratch / 'SM_SplatmapAudit_UVCorrected_YawMinus90.fbx'
report_path = scratch / 'audit-corrected-fbx-semantic.json'
verify_existing = '--verify-existing' in sys.argv
assert (verify_existing or not destination.exists()) and not report_path.exists()
original_digest = hashlib.md5(source.read_bytes()).hexdigest()
assert original_digest == '18ba5d2bc52b9808e6c2f7f522e18417'

module_folder = Path('C:/Program Files/Blender Foundation/Blender 5.0/5.0/scripts/addons_core/io_scene_fbx')
parse = runpy.run_path(str(module_folder / 'fbx2json.py'), run_name='fbx_reader')['parse']
sys.path.insert(0, str(module_folder))
import encode_bin
import data_types

root, version = parse(str(source))
assert version == 7300
selected = []
methods = {
    'I': 'add_int32', 'S': 'add_string', 'R': 'add_bytes',
    'D': 'add_float64', 'L': 'add_int64', 'd': 'add_float64_array',
    'i': 'add_int32_array', 'C': 'add_char',
}


def clone(element, ancestors=()):
    path = ancestors + (element.id.decode(),)
    output = encode_bin.FBXElem(element.id)
    for kind, value in zip(element.props_type, element.props):
        if path in (
            ('', 'Objects', 'Geometry', 'LayerElementUV', 'UV'),
            ('', 'Objects', 'Geometry', 'LayerElementBumpUV', 'UV'),
        ):
            assert chr(kind) == 'd' and len(value) == 8192
            value = array.array('d', (1.0 - item if index % 2 else item for index, item in enumerate(value)))
            selected.append('/'.join(path))
        if chr(kind) == 'i':
            value = array.array(data_types.ARRAY_INT32, value)
        getattr(output, methods[chr(kind)])(value)
    output.elems = [clone(child, path) for child in element.elems]
    return output


encoded = clone(root)
assert len(selected) == 2
if not verify_existing:
    encode_bin.write(str(destination), encoded, version)
# This bundled parser initializes its format globals once per namespace.
fresh_parse = runpy.run_path(str(module_folder / 'fbx2json.py'), run_name='fbx_verifier')['parse']
actual, actual_version = fresh_parse(str(destination))
assert actual_version == version
differences = []


def compare(before, after, ancestors=()):
    assert before.id == after.id and before.props_type == after.props_type
    assert len(before.props) == len(after.props) and len(before.elems) == len(after.elems)
    path = ancestors + (before.id.decode(),)
    name = '/'.join(path)
    for old, new in zip(before.props, after.props):
        if old == new:
            continue
        differences.append(name)
        if name in selected:
            expected = array.array('d', (1.0 - item if index % 2 else item for index, item in enumerate(old)))
            assert new == expected
        else:
            assert path in (('', 'FileId'), ('', 'CreationTime')), name
    for old_child, new_child in zip(before.elems, after.elems):
        compare(old_child, new_child, path)


compare(root, actual)
assert hashlib.md5(source.read_bytes()).hexdigest() == original_digest
report = {
    'source': str(source), 'source_md5_before_and_after': original_digest,
    'scratch_fbx': str(destination), 'fbx_version': version,
    'changed_property_paths': differences,
    'all_other_parsed_properties_identical': True,
    'geometry_and_model_transforms_unchanged': True,
    'planned_import_yaw_degrees': -90,
    'writer_note': 'Binary layout/compression/footer rewritten; FileId and CreationTime may change.',
}
report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report))
