from pathlib import Path
import importlib.util
import tempfile

tool = Path(r'C:\Users\tarik.bergstrom\Perforce\AGP-Content\Tools\Unreal\ExportAndPlay')
target = Path(__file__).parent / 'ExportAndPlayModule.py'
source = (tool / target.name).read_text()
for name in ('fromPath', 'toPath', 'gamePath'):
    source = source.replace(f'{name} = file.readline()', f'{name} = file.readline().strip()')
start = source.index('    file = AddBackSlashAtStart(aFileToMove)')
end = source.index('\n    print("Copied: "', start)
source = source[:start] + '''    sourceFilePath = Path(aFileToMove)
    if not sourceFilePath.is_absolute():
        sourceFilePath = Path(aPaths[0]) / sourceFilePath
    if not sourceFilePath.is_file():
        print(f"- Couldn't find file to move: '{sourceFilePath}'")
        return False

    fileNameWithPrefix = "EAP.json"
    destination = Path(aPaths[1]) / fileNameWithPrefix
    shutil.copyfile(sourceFilePath, destination)
''' + source[end:]
start = source.index('    filetypes = ')
end = source.index('\n\n\n', start)
source = source[:start] + '''    root = tkinter.Tk()
    root.withdraw()
    try:
        selectedFile = filedialog.askopenfilename(
            initialdir=str(localPaths[0]), filetypes=(("JSON files", "*.json"),))
    finally:
        root.destroy()
    if not selectedFile:
        return False
    return MoveFile(selectedFile, localPaths)''' + source[end:]
source = source.replace('    MoveLevelFile()\n    PlayInGameEngine()', '    if not MoveLevelFile():\n        return False\n    return PlayInGameEngine()')
target.write_text(source)
spec = importlib.util.spec_from_file_location('export_and_play', target)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as directory:
    fixture = Path(directory)
    nested = fixture / 'exports' / 'nested'
    nested.mkdir(parents=True)
    destination = fixture / 'game-content'
    destination.mkdir()
    scene = nested / 'Level.json'
    scene.write_text('{"Actors": []}')
    paths = (str(nested.parent), str(destination), str(fixture))
    assert module.MoveFile(str(scene), paths)
    assert (destination / 'EAP.json').read_bytes() == scene.read_bytes()
    assert not module.MoveFile('missing.json', paths)
    settings = []
    for name, value in zip(('from', 'to', 'game'), paths):
        path = fixture / (name + '.txt')
        path.write_text(value + '\n')
        settings.append(path)
    assert module.QueryPaths(settings) == paths
    module.MoveLevelFile = lambda: False
    module.PlayInGameEngine = lambda: (_ for _ in ()).throw(AssertionError('Launched after failed transfer'))
    assert module.ExportAndPlay() is False
print('PASS: nested scene transfer, exact EAP filename/content, missing source, path whitespace, failed-transfer launch guard')
