# Runtime DLLs

The Game prebuild step copies these DLLs beside `Game.exe`. These are the files
from the team's existing Windows workspace, recorded here so a Git clone has
the same build inputs. The hashes identify their exact bytes; they do not
establish where the files were originally obtained or grant redistribution
rights.

| File | Product version | SHA-256 |
| --- | --- | --- |
| `fmod.dll` | 2.2.5 | `E567DDD8029A02A0239DC779E15A04427C60039D514E217BA14863CDF1A65064` |
| `fmodL.dll` | 2.2.5 | `0737E9FA71D59E3DD32C72F1BE2F3D71C2B9FE4C3C199CD9AEEF9F2FD48ED2D5` |
| `fmodstudio.dll` | 2.2.5 | `9F07AF448DD381EA83CC952699EBB57D5D635F5C5F63872CD4CB0BF1A668B6D5` |
| `fmodstudioL.dll` | 2.2.5 | `3A4667E9825FB04DDDEFA5BC4E9260C28F9A60C8C7945E4B35202EF827A0E833` |
| `libfbxsdk.dll` | 2020.3.4 Release | `0C152E1EB554C5F3C526B60E465FA701371179CB6708664AC021D6588FA70D89` |

Only the files listed above are allowed through `.gitignore`. Keep the vendor
SDK files and licence review separate from these runtime inputs.
