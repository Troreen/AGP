# Three-layer terrain splat material

Status: in-progress

## Problem Statement

The authored terrain painting and three complete snow surface layers do not
reach AGP. Existing suffix-based texture binding overwrites multiple layers.
The terrain asset must retain its intentional -90 degree import rotation while
removing an unwanted UV V reflection.

## Solution

One opaque lit terrain material blends Dunes, Rough and Cliff base color, linear
ORM and normal detail. Terrain and ground floaters share world-space placement,
8 metre surface repeats, the native Landscape phase and one level-covering splat
mask. A linear noise texture reshapes transitions with strength and patch-size
controls. Strength zero provides the comparison baseline.

## User Stories

1. As an artist, I want all three complete surface layers to render independently.
2. As an artist, I want painted Rough and Cliff over a continuous Dunes base.
3. As an artist, I want adjustable noisy transitions without new absent layers.
4. As an artist, I want floaters to match terrain at the same world position.
5. As an artist, I want the required mesh rotation preserved and UV reflection removed.
6. As a developer, I want existing materials to retain their legacy bindings.
7. As an artist, I want preparation to detect geometry outside mask coverage.

## Implementation Decisions

- Owner authorized implementation on 2026-10-22. No staging, commits, publication
  or Perforce submit. Preserve the initial application settings, research and all
  existing content changes, including the modified Unreal Landscape master/map.
- Fixed three layers: Dunes base, ordered Rough then Cliff alpha overlays. Do not
  normalize independent painted masks. The Dunes mask remains diagnostic data;
  filling uncovered contribution makes it redundant for final coverage.
- Texture noise, with adjustable strength and patch size; noise changes only
  weights strictly between zero and one. Use one shared world coordinate mapping.
- The splat mask must cover the entire authored level. Coverage failures are
  preparation/validation failures, not an intended outside-area painting policy.
- Use native Unreal XY axes and native Landscape surface phase. AGP projection
  uses world ZX. Import yaw -90 remains part of geometry preparation, not a
  shader compensation. Surface tiling and mask coverage are distinct transforms.
- Splat channels R/G/B are Dunes/Rough/Cliff, linear numeric data; noise is linear
  too. Existing sRGB noise requires explicit linear preparation, not a format
  assumption. Runtime masks use lossless RGBA8 UNORM with mipmaps.
- Use existing exported scalar/vector/texture records. Explicit reflected texture
  parameter names take priority over legacy asset suffix/name fallbacks.
- Correct reflected constant-buffer offsets to honor HLSL layout. Validate
  material data within the existing 128-byte and 16-texture caps.
- Game shader/assets and repeatable preparation tools belong to AGP-Content;
  engine material binding/reflection changes belong to AGP. Use distinct new
  runtime assets/verification scene so existing Unreal edits need not be saved.
- Project normal detail in a world-aligned, slope-preserving frame. A flat normal
  must preserve geometric slope; rotated floaters must retain world detail axes.

## Testing Decisions

- Existing MaterialImportTests scene-building/material interfaces exercise named
  texture priority, instance overrides, legacy fallbacks and reflected packing.
  They do not prove visual correctness.
- Compile the content material through real engine entry points/reflection; add
  GPU readback cases for ordered weights, endpoint-preserving noise and projected
  normals where practical. Expected outputs come from fixed worked examples.
- Numeric preparation checks cover PNG/DDS channel equality, mip dimensions,
  mask registration, source checksums, geometry and intentional rotation.
- Runtime verification scene: asymmetric terrain landmarks, three surface sets,
  matching and rotated floaters, measured 800-unit repeat, pure/overlap/unpainted
  areas, noise off/on, all surface debug views, near/far and GPU timing. Required
  checks without evidence remain pending; builds are not visual verification.

## Out of Scope

Native Landscape geometry import, arbitrary layer counts, height blending,
triplanar mapping, terrain editing UI, automatic graph translation and unrelated
asset cleanup. Do not save the already dirty Unreal level or overwrite its master.

## Further Notes

Current source evidence and prior approvals are in investigation-summary.md,
asset-correction-procedure.md and workflow.md. The approved scratch correction
already retained identical geometry/topology with yaw -90 and corrected all UVs.
