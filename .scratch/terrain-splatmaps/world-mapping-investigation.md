# World-space splat mapping investigation

Planning investigation, 2026-10-22. Source inspection only; no shader or runtime changes. Unreal editor and visual checks remain pending.

## Confirmed from local source

- The scene component serializer writes the Unreal relative transform matrix directly, row by row, without a unit conversion: [TGALevelSerializer.cpp](../../../AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp), lines 538–558.
- The active AGP importer permutes both actor and component transforms. Translation becomes `(Unreal Y, Unreal Z, Unreal X)`: [UnrealSceneImporter.cpp](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp), lines 136–153 and 342–364. Therefore Unreal's horizontal XY coordinates correspond to AGP **ZX**, in that order, and Unreal up Z corresponds to AGP Y. Using `.xy` in an AGP shader would include height; `.xz` selects the ground plane but swaps the requested Unreal XY orientation.
- Runtime transform conversion retains the imported numeric translation scale: `UnrealUnitScale = 1.0f`, then `transform.Position = position * UnrealUnitScale`: [UnrealSceneImporter.cpp](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp), lines 560–575. No metres conversion occurs here.
- The plugin's separate navmesh exporter explicitly declares centimetres and Unreal Z-up before converting to Maya Y-up: [TGAUnrealPlugin.cpp](../../../AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGAUnrealPlugin.cpp), lines 419–422 and 449. This supports the content pipeline's centimetre convention, but is not proof of the museum render mesh's FBX settings.
- The shader receives actual transformed world position, so shared mapping does not require mesh-specific UVs: [Surface_VS.hlsl](../../Source/Engine/GraphicsEngine/Shaders/Material/Surface_VS.hlsl), lines 35–49; [GBuffer_PS.hlsl](../../Source/Engine/GraphicsEngine/Shaders/Material/GBuffer_PS.hlsl), lines 30–42. The pixel hook also receives vertex normal, tangent and binormal.
- The mesh loader calls the prebuilt TGA FBX importer and copies its resulting positions, normals, tangent and UVs without another axis/unit conversion: [MeshAsset.cpp](../../Source/Engine/GameFramework/AssetHandling/MeshAsset.cpp), lines 27–30 and 63–66; [AssetLoadingHelper.cpp](../../Source/Engine/GameFramework/AssetHandling/AssetLoadingHelper.cpp), lines 56–66 and 98–110. The importer implementation is not available in this repository, only headers and libraries; its internal FBX conversions cannot be established from these call sites.

## Planning consequences and proposed contract

These are design deductions, not implemented behavior:

1. Define horizontal coordinates in **Unreal XY order**, represented by AGP `WorldPosition.zx`. For the current unscaled scene-transform route and centimetre-authored content, the expected surface mapping is `WorldPosition.zx / 800`: one repeat per 8 metres. Keep 800 provisional until the measured museum test verifies FBX geometry and scene transforms together. Do not divide AGP position by 8 under an assumption that its units are metres.
2. Give the repeating surface textures one common world origin/phase and repeat distance. Base color, ORM and normal maps within each layer must use identical coordinates. Terrain and floaters must use the same values regardless of their mesh transform.
3. Give the mask an independent shared origin and world coverage extent. Conceptually, mask UV is `(horizontalPosition - maskOrigin) / maskExtent`, followed by documented orientation/flips. Do not reuse the 8-metre repeat for a terrain-wide authored mask. Record outside-coverage behavior after inspecting the source mask.
4. A vertically displaced floater shares terrain weights underneath it because the projection ignores height. A rotated or scaled floater must still sample the same world coordinates at the same physical points.

## Projected normal-map implications

The current normal helper decodes two texture channels to tangent space and multiplies by the **mesh-derived** tangent/binormal/normal frame: [Lighting.hlsli](../../Source/Engine/GraphicsEngine/Shaders/Material/Lighting.hlsli), lines 196–217. That frame is currently built from imported mesh directions and transformed by the object matrix: [Surface_VS.hlsl](../../Source/Engine/GraphicsEngine/Shaders/Material/Surface_VS.hlsl), lines 54–58.

Changing only normal texture UVs to world projection would leave this frame tied to mesh UV orientation. Normal details could rotate independently of the world-aligned color on rotated floaters. Plan an explicit projection-consistent normal basis/reorientation against the geometric normal; preserve geometric slope and confirm normal channel orientation. The pixel hook can write a final world-space `parameters.Normal`, which the GBuffer already outputs ([GBuffer_PS.hlsl](../../Source/Engine/GraphicsEngine/Shaders/Material/GBuffer_PS.hlsl), lines 40 and 54). This can fit the existing material seam, but the precise method should be selected during shader design.

Planar mapping also compresses texture detail on steep sides. A steep/rotated floater should be included in visual verification; triplanar mapping is a possible later extension, not an established requirement for this MVP.

## What still needs Unreal or runtime verification

- Museum terrain mask origin, coverage, XY orientation and any V flip; whether it is a texture or Landscape layer data.
- One measured 8-metre span: verify one texture repeat and no discrepancy between mesh dimensions and scene translation units.
- Marker/checker comparison between Unreal XY and AGP ZX, including mask corners, not merely visually plausible tiling.
- Terrain plus translated, rotated and scaled floater at a painted material boundary: matching color scale, mask region and directional normal detail.
- A flat normal map must preserve geometric terrain/floater slope; a directional normal test must expose swapped or inverted axes.

No editor result, normal-map correctness, mask alignment or render-mesh FBX unit convention has been claimed as verified.
