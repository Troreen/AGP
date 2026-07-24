# Static-Mesh Artifacts

`AGPTools` owns AGP's derived static-mesh artifact. The library is optional: it
has no editor dependency and does not create graphics-device resources. This
keeps it useful to AGP command-line tools, tests, ModelViewer integration, and
other offline consumers.

Include `Source/Tools/AGPTools/StaticMeshArtifact.h` and use:

- `WriteStaticMeshArtifact` to serialize and then revalidate a complete staged
  artifact;
- `ValidateStaticMeshArtifact` to check an existing artifact without exposing
  mesh data; and
- `ReadStaticMeshArtifact` to validate first and return CPU mesh data only after
  every structural check succeeds.

`GetStaticMeshToolVersion()` and `StaticMeshArtifactSchemaVersion` are the
provenance inputs for callers that cache derived output. Failures use stable
diagnostic codes, a severity, a byte offset, and a readable message.

Schema version 1 is a little-endian binary containing a fixed header and exactly
three ordered sections: static vertices, 32-bit indices, and submesh ranges. The
byte layout is an AGP implementation detail in `StaticMeshArtifactFormat.h`;
callers outside AGP should treat the file as opaque and use the public API.

The structural validator rejects unsupported magic or versions, truncated or
trailing bytes, unbounded counts, incorrect section kinds/strides/sizes, gaps or
overlaps, non-finite vertex components, out-of-range indices, invalid submesh
ranges, and unsupported material indices. The reader never returns partial mesh
data from an invalid artifact.

Build and run the focused Release tests from the AGP root with:

```powershell
msbuild AGP.sln /m /t:AGPToolsTests /p:Configuration=Release /p:Platform=x64
.\Bin\Release\AGPToolsTests.exe
```
