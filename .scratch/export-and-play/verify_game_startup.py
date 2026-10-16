from pathlib import Path
import importlib.util
import hashlib
import subprocess
import time
import sys

workspace = Path(__file__).resolve().parents[2]
tool = Path(r'C:\Users\tarik.bergstrom\Perforce\AGP-Content\Tools\Unreal\ExportAndPlay\ExportAndPlayModule.py')
spec = importlib.util.spec_from_file_location('eap', tool)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
paths = module.QueryPaths(module.FindDirectories())
destination = Path(paths[1]) / 'EAP.json'
assert destination.resolve() == (workspace / 'Content' / 'EAP.json').resolve()
backup = Path(__file__).parent / 'EAP.failed-import.json'
if destination.exists() and not backup.exists():
    backup.write_bytes(destination.read_bytes())
source = Path(paths[0]) / (sys.argv[1] if len(sys.argv) > 1 else 'lvl_blockout/Lvl_Blockout_Level.json')
assert module.MoveFile(str(source), paths)
assert hashlib.sha256(source.read_bytes()).digest() == hashlib.sha256(destination.read_bytes()).digest()
print('PASS: installed tool copied exact selected nested scene', flush=True)
game = subprocess.Popen([str(Path(paths[2]) / 'Game.exe')], cwd=workspace)
print(f'Launched Game PID {game.pid}', flush=True)
time.sleep(15)
print(f'Game exit code after 15 seconds: {game.poll()}', flush=True)
print((workspace / 'Game.log').read_text(errors='replace')[-7000:], flush=True)
