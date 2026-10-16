from pathlib import Path
import importlib.util
import tempfile
import difflib

directory = Path(__file__).resolve().parent
tool = Path(r'C:\Users\tarik.bergstrom\Perforce\AGP-Content\Tools\Unreal\ExportAndPlay\ExportAndPlayModule.py')
original = tool.read_text()
fixed = original.replace(
    '    file = AddBackSlashAtStart(aFileToMove)\n    fileQuery = Path(aPaths[0] + file)',
    '    fileQuery = Path(aPaths[0]) / aFileToMove', 1)
fixed = fixed.replace('    sourceFilePath = os.path.join(aPaths[0], aFileToMove)',
                      '    sourceFilePath = fileQuery', 1)
fixed = fixed.replace('    # Save file name\n    LevelFileName = os.path.basename(folder_path)\n\n    return MoveFile(LevelFileName, localPaths)',
                      '    if not folder_path:\n        return False\n\n    return MoveFile(folder_path, localPaths)', 1)
assert fixed != original
target = directory / 'ExportAndPlayModule.small.py'
target.write_text(fixed)
(directory / 'ExportAndPlayModule.before-small-fix.py').write_text(original)
print(''.join(difflib.unified_diff(original.splitlines(True), fixed.splitlines(True), fromfile='original', tofile='fixed')))

def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

with tempfile.TemporaryDirectory(dir=directory) as temporary:
    fixture = Path(temporary)
    source = fixture / 'exports'
    nested = source / 'nested'
    nested.mkdir(parents=True)
    destination = fixture / 'content'
    destination.mkdir()
    (source / 'Level.json').write_text('old scene')
    selected = nested / 'Level.json'
    selected.write_text('selected scene')
    paths = (str(source), str(destination), str(fixture))
    old = load(tool, 'old_tool')
    assert old.MoveFile(selected.name, paths)
    assert (destination / 'EAP.json').read_text() == 'old scene'
    new = load(target, 'fixed_tool')
    assert new.MoveFile(str(selected), paths)
    assert (destination / 'EAP.json').read_text() == 'selected scene'
    assert new.MoveFile('Level.json', paths)
    assert (destination / 'EAP.json').read_text() == 'old scene'
    new.FindDirectories = lambda: None
    new.QueryPaths = lambda _: paths
    class Window:
        def withdraw(self): pass
    new.tkinter.Tk = Window
    new.filedialog.askopenfilename = lambda **_: str(selected)
    assert new.MoveLevelFile()
    assert (destination / 'EAP.json').read_text() == 'selected scene'
    new.filedialog.askopenfilename = lambda **_: ''
    assert new.MoveLevelFile() is False
print('PASS: reproduced wrong-file copy; fixed nested selection, relative filename compatibility, picker forwarding, and cancellation')
