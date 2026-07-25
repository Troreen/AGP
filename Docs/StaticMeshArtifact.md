# AGP Static-Mesh Tooling

`AGPTools` owns AGP's derived static-mesh conversion and `.agpmesh` artifact.
It is an optional static library with no editor dependency and no graphics-device
resource creation. AGP command-line tools, ModelViewer, tests, and external
consumers all use the same conversion behavior.

The caller owns source staging, asset identity, import settings, cache placement,
provenance hashes, destination collision policy, and final atomic commit. AGPTools
only reads one FBX, converts supported static render data, writes one caller-owned
staged artifact, and validates that complete artifact.

## Public API

Include `AGP/Tools/StaticMeshFbx.h` from a staged bundle:

```cpp
#include <AGP/Tools/StaticMeshFbx.h>

#include <atomic>

std::atomic_bool canceled = false;
const auto result = AGP::Tools::BuildStaticMeshArtifactFromFbx(
    sourceFbx,
    stagedArtifact,
    canceled);

if (!result.Succeeded()) {
    for (const auto& diagnostic : result.Diagnostics) {
        // diagnostic.Code, Severity, Message, SourcePath, and ByteOffset
    }
}
```

The supported operations are:

- `ConvertFbxToStaticMeshData` for CPU-side static mesh conversion;
- `BuildStaticMeshArtifactFromFbx` for import, conversion, artifact write, and
  final on-disk validation;
- `WriteStaticMeshArtifact`, `ValidateStaticMeshArtifact`, and
  `ReadStaticMeshArtifact` for artifact-only workflows; and
- `GetStaticMeshToolVersion` plus `StaticMeshArtifactSchemaVersion` for cache
  provenance.

V1 accepts ordinary static FBX mesh elements. The importer preserves the existing
AGP DirectX axis conversion, centimeter unit conversion, UV0/UV1, normalized
normal/tangent behavior, first vertex-color channel with opaque-white fallback,
absolute indices, element ranges, and material indices. Empty elements, malformed
indices, non-finite data, skinning, skeletons, and LOD groups fail with stable
diagnostics. Animation remains a separate ModelViewer capability.

The tool version changes when conversion behavior or the public tooling contract
changes and therefore participates in cache invalidation. The artifact schema
version changes only when the binary layout or structural interpretation changes.
The current values are `agp-static-mesh-tool/1.1.0` and artifact schema `1`.

## Cancellation and importer lifetime

The bundled TGA/FBX SDK call is not interruptible. AGPTools checks the cooperative
atomic cancellation flag before waiting for/importing and immediately after the
SDK call returns. A canceled conversion exposes no mesh. If cancellation races
with an already completed artifact write, the result is canceled and the caller
must discard its staging path rather than commit it.

The underlying importer owns process-global mutable state. AGPTools initializes it
on first use, releases it at process shutdown, and serializes public FBX imports.
Concurrent AGPTools callers are safe but execute their SDK calls one at a time.
Calling the raw third-party importer concurrently is outside the supported API.

## Artifact validation

Schema 1 is a little-endian fixed header followed by exactly three ordered
sections: static vertices, 32-bit indices, and submesh ranges. Its byte layout is
an AGP implementation detail; external callers treat it as opaque.

Validation rejects unsupported magic or versions, truncation or trailing bytes,
unbounded counts, incorrect section kinds/strides/sizes, gaps or overlaps,
non-finite vertex components, out-of-range indices, invalid submesh ranges, and
unsupported material indices. The reader returns no partial mesh for an invalid
artifact.

## Deterministic staged bundle

From the AGP root, export one configuration with:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\StageAGPTools.ps1 -Configuration Release
```

The command builds AGPTools for MSVC v145 x64, removes only the selected generated
bundle directory, copies the exact supported payload, writes SHA-256/length/version
metadata, and verifies every staged hash. Replace `Release` with `Debug` for the
debug bundle.

The output contract is `Artifacts/AGPTools/<Configuration>/x64/`:

```text
AGPToolsBundle.json
include/AGP/Tools/StaticMeshArtifact.h
include/AGP/Tools/StaticMeshFbx.h
lib/AGPTools.lib
lib/libfbxsdk.lib
lib/libxml2-md.lib
lib/zlib-md.lib
bin/libfbxsdk.dll
```

Consumers add `include` to their include path, link all four libraries from `lib`,
and place `bin/libfbxsdk.dll` beside the consuming executable. They do not search
the AGP checkout, `Lib`, or `ThirdParty` at runtime. The JSON manifest is the exact
bundle inventory and identifies each file's role, byte length, and SHA-256.

## Focused verification

This checkout's Visual Studio 2026 installation is invoked explicitly below. The
clean child `cmd` avoids the machine's duplicate `Path`/`PATH` environment issue:

```powershell
cmd.exe /d /c 'set "PATH=" & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" Source\Tools\AGPToolsTests\AGPToolsTests.vcxproj /m /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir=C:\Users\tarik\Documents\GitHub\AGP\ /v:minimal'
.\Bin\Debug\AGPToolsTests.exe

cmd.exe /d /c 'set "PATH=" & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" Source\Tools\AGPToolsTests\AGPToolsTests.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=C:\Users\tarik\Documents\GitHub\AGP\ /v:minimal'
.\Bin\Release\AGPToolsTests.exe

powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Tools\Tests\Test-StageAGPTools.ps1
```

The C++ tests retain the artifact corruption suite and exercise the real
`SM_Chest.fbx`, exact deterministic counts/ranges, direct FBX-to-artifact output,
read-back, missing/malformed/empty/skeletal failures, and both cancellation points.
The PowerShell test verifies both configuration manifests, hashes, and exact file
layouts.
