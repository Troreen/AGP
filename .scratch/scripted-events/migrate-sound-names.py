import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / 'Source/Engine/GameFramework/AudioManager.h').read_text()
enum_body = re.search(r'enum SoundID\s*\{(.*?)\};', header, re.S).group(1)
names = re.findall(r'\b(e\w+)\b', enum_body)
names.remove('eSoundCount')
paths = list((root / 'Content/ExportedScenes').rglob('*.json'))
paths += list((root / '.scratch/scripted-events').glob('*sequence*.json'))
for path in paths:
    text = path.read_bytes().decode('utf-8-sig')
    def replace_sound(match):
        value = int(match.group(2))
        if not 0 <= value < len(names):
            raise ValueError((path, value))
        return match.group(1) + json.dumps(names[value])
    updated = re.sub(r'("Sound"\s*:\s*)(\d+)\b', replace_sound, text)
    if updated != text:
        path.write_bytes(updated.encode('utf-8'))
        print('Migrated', path.relative_to(root))

path = root / 'Docs/ScriptedEvents.md'
text = path.read_bytes().decode('utf-8-sig')
text = text.replace('| `StringVariables` | Names such as `TargetActor`, `DestinationActor`, `Variable` |', '| `StringVariables` | Names such as `TargetActor`, `DestinationActor`, `Variable`, `Sound` |')
text = text.replace('| `IntVariables` | `Step`, `Sound` |', '| `IntVariables` | `Step` |')
text = text.replace('using int `Sound` (see sound IDs below)', 'using string `Sound`, for example `eSlash` (see sound names below)')
text = text.replace('`Sound=<registered dialogue ID>`', '`Sound=<registered dialogue enum name>`')
text = text.replace('## Where to find sound IDs and animation names', '## Where to find sound names and animation names')
start = text.index('**Sounds:**')
end = text.index('\n\n', start)
text = text[:start] + '**Sounds:** In `StringVariables`, add `Sound` and use an exact enum name from [`AudioManager.h`](../Source/Engine/GameFramework/AudioManager.h), such as `eSlash`, `eSlashHit`, or `eHeavyRoar`. Names are case-sensitive. Do not put `Sound` in `IntVariables`; numeric IDs are no longer accepted. Do not use `eSoundCount`. [`AudioManager.cpp`](../Source/Engine/GameFramework/AudioManager.cpp) maps these enum values to FMOD events. New sounds must be registered there and added to the scripted sound-name mapping in [`ScriptedActionComponent.cpp`](../Source/Application/Game/Components/ScriptedEvents/ScriptedActionComponent.cpp). Avoid overlapping actions using the same sound: they share one playback.' + text[end:]
text = text.replace('`Sound=12`', '`Sound=eSlash`')
path.write_bytes(text.encode('utf-8'))

path = root / 'Docs/ScriptedEventsUnrealTestGuide.md'
text = path.read_bytes().decode('utf-8-sig')
text = text.replace('Sound=5', 'Sound=`eCollectItem`').replace('Sound=7', 'Sound=`eHeavyRoar`')
text = text.replace('The current [SoundID enum](../Source/Engine/GameFramework/AudioManager.h) assigns **5 to eCollectItem** and **7 to eHeavyRoar**. Both are registered by AudioManager. These are integer IDs, not Unreal sound assets or FMOD event paths. Recheck the enum if someone changes its ordering.', 'Use the exact [SoundID enum names](../Source/Engine/GameFramework/AudioManager.h): `eCollectItem` and `eHeavyRoar`. Put `Sound` in `StringVariables`, not `IntVariables`. Both sounds are registered by AudioManager; these names are not Unreal sound assets or FMOD event paths.')
path.write_bytes(text.encode('utf-8'))
