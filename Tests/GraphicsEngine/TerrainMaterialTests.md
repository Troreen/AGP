# Terrain material GPU checks

This standalone executable compiles an authored material and calls its public
`Material_Pixel` hook in a real D3D11 pixel shader. WARP renders one pixel into
three floating-point targets; CPU readback checks base color, ORM and world
normal against fixed worked examples. It does not link or launch the game.

From the repository root:

```powershell
& 'C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe' Tests/GraphicsEngine/TerrainMaterialTests.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo /nr:false
& ./Bin/Tests/Debug/TerrainMaterialTests.exe <path-to-MM_Landscape.hlsli> Source/Engine/GraphicsEngine/Shaders/Material
```

The fixture binds distinct red/green/blue surface layers through the material's
texture slots and its exported b4 constants. A Rough weight of 0.5 followed by
a Cliff weight of 0.25 must produce `(0.375, 0.375, 0.25)` in color and ORM.
Two-texel fixtures distinguish world position, surface repetition, mask mapping
and noise patch size. A decoded `(0.6, 0, 0.8)` detail normal verifies the shared
world frame; flat and cancelling detail normals verify slope preservation and
the safe fallback. No expected value is obtained from a material helper.

The point sampler and single pixel isolate sampling coordinates and material
arithmetic. These checks do not validate authored assets, filtering or mip
appearance, real scene lighting, draw-time engine bindings, geometry import or
hardware performance. The feature's runtime visual and timing checks remain
required separately.
