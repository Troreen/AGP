"""Precompensate Landscape FBX UV V before Unreal import; preserve geometry.

Run with Blender's bundled Python. Requires its io_scene_fbx modules, not a DCC
scene import/re-export. Import the result into Unreal with the required yaw -90,
then use ExportTerrainRuntime.py (Force Front X) for AGP runtime geometry.
"""

import argparse
import array
import hashlib
import json
from pathlib import Path
import runpy
import sys


def correct(source, destination, module_directory):
    source, destination = source.resolve(), destination.resolve()
    provenance = destination.with_suffix('.provenance.json')
    if source == destination or destination.exists() or provenance.exists():
        raise ValueError('Use a new destination; source and existing outputs are preserved')
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    parse = runpy.run_path(str(module_directory / 'fbx2json.py'), run_name='fbx_source')['parse']
    sys.path.insert(0, str(module_directory))
    import encode_bin
    import data_types

    root, version = parse(str(source))
    selected = []
    methods = {'I': 'add_int32', 'S': 'add_string', 'R': 'add_bytes',
               'D': 'add_float64', 'L': 'add_int64', 'd': 'add_float64_array',
               'i': 'add_int32_array', 'C': 'add_char'}
    uv_paths = {('', 'Objects', 'Geometry', 'LayerElementUV', 'UV'),
                ('', 'Objects', 'Geometry', 'LayerElementBumpUV', 'UV')}

    def reflected_values(values):
        return array.array('d', (1 - value if index % 2 else value for index, value in enumerate(values)))

    def clone(element, ancestors=()):
        path = ancestors + (element.id.decode(),)
        output = encode_bin.FBXElem(element.id)
        for kind, value in zip(element.props_type, element.props):
            if path in uv_paths:
                if chr(kind) != 'd' or not value or len(value) % 2:
                    raise ValueError('Expected paired double-precision Landscape UV values')
                value = reflected_values(value)
                selected.append(path)
            if chr(kind) == 'i':
                value = array.array(data_types.ARRAY_INT32, value)
            getattr(output, methods[chr(kind)])(value)
        output.elems = [clone(child, path) for child in element.elems]
        return output

    encoded = clone(root)
    if len(selected) != 2 or set(selected) != uv_paths:
        raise ValueError('Expected exactly the Landscape surface and weightmap UV arrays')
    destination.parent.mkdir(parents=True, exist_ok=True)
    encode_bin.write(str(destination), encoded, version)
    fresh_parse = runpy.run_path(str(module_directory / 'fbx2json.py'), run_name='fbx_output')['parse']
    actual, actual_version = fresh_parse(str(destination))
    if actual_version != version:
        raise ValueError('FBX version changed')
    differences = []

    def compare(before, after, ancestors=()):
        path = ancestors + (before.id.decode(),)
        if (before.id != after.id or before.props_type != after.props_type
                or len(before.props) != len(after.props) or len(before.elems) != len(after.elems)):
            raise ValueError('FBX tree shape or property types changed: ' + '/'.join(path))
        for old, new in zip(before.props, after.props):
            if old == new:
                continue
            differences.append('/'.join(path))
            if path in uv_paths:
                if new != reflected_values(old):
                    raise ValueError('Unexpected UV change')
            elif path not in {('', 'FileId'), ('', 'CreationTime')}:
                raise ValueError('Unexpected non-UV change: ' + '/'.join(path))
        for old, new in zip(before.elems, after.elems):
            compare(old, new, path)

    compare(root, actual)
    if hashlib.sha256(source.read_bytes()).hexdigest() != digest:
        raise ValueError('Original source changed during preparation')
    record = {'source_file': source.name, 'source_sha256': digest,
              'output_sha256': hashlib.sha256(destination.read_bytes()).hexdigest(),
              'fbx_version': version, 'changed_property_paths': differences,
              'all_other_parsed_properties_identical': True,
              'unreal_import_yaw_degrees': -90, 'runtime_export_force_front_x_axis': True,
              'note': 'Writer also rewrites binary offsets/compression/footer; not a byte-only UV patch.'}
    provenance.write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    print('Verified UV-corrected FBX: ' + str(destination))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--fbx-module-directory', type=Path, required=True)
    arguments = parser.parse_args()
    correct(arguments.source, arguments.destination, arguments.fbx_module_directory.resolve())
