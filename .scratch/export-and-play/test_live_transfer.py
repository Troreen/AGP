import hashlib
import importlib.util
from pathlib import Path
import sys
import time

tool = Path(r'C:\Users\tarik.bergstrom\Perforce\AGP-Content\Tools\Unreal\ExportAndPlay\ExportAndPlayModule.py')
spec = importlib.util.spec_from_file_location('live_export_and_play', tool)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
paths = module.QueryPaths(module.FindDirectories())
assert paths, 'Invalid tool path configuration'
destination = Path(paths[1].strip()) / 'EAP.json'
workspace = Path(__file__).resolve().parents[2]
assert destination.parent.resolve() == (workspace / 'Content').resolve(), 'Destination is outside workspace content'
if '--launch' in sys.argv:
    assert module.PlayInGameEngine(), 'Game launch failed'
else:
    if destination.exists():
        backup = Path(__file__).parent / 'EAP.before-test.json'
        assert not backup.exists(), 'Backup already exists; preserve it before repeating test'
        backup.write_bytes(destination.read_bytes())
    selected = Path(paths[0].strip()) / 'lvl_blockout' / 'Lvl_Blockout_Level.json'
    actual = Path(paths[0]) / selected.name
    print(f'UI selected file: {selected}', flush=True)
    print(f'Tool resolves to: {actual}', flush=True)
    print(f'Selected SHA256: {hashlib.sha256(selected.read_bytes()).hexdigest()}', flush=True)
    assert module.MoveFile(selected.name, paths), 'Transfer failed'
    print(f'Copied SHA256: {hashlib.sha256(destination.read_bytes()).hexdigest()}', flush=True)
for second in range(11):
    if destination.exists():
        print(f'{second}s: EAP.json exists, {destination.stat().st_size} bytes', flush=True)
    else:
        print(f'{second}s: EAP.json MISSING', flush=True)
    if second < 10:
        time.sleep(1)
