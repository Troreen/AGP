# Scene importer integration concerns

This document preserves historical handoff assumptions, not a verified current Perforce contract. The actual scene importer, exporter guide and representative exports were unavailable in this checkout. M5 is blocked on the precise inputs recorded in [implementation evidence](SimplifiedGameFrameworkImplementation.md).

The implemented AGP boundary is Integration/GameFramework/Integration/ISceneSource.h: a source returns owned SceneData or source-addressed errors and supplies ready resource bindings at the synchronous host loading point. ModelViewer and regression fixtures use this same path. No importer schema or coordinate conversion is inferred from these tests.

## Responsibility boundary

The importer parses JSON into descriptions. The engine-side builder handles runtime
construction, registered component factories, reference resolution and activation.
Coordinate the data contract before either side finalizes its implementation.
Asset resolution is TBD with the team responsible for asset management.

## 1. Transform semantics - highest priority

- The supplied exporter guide describes component Transform as relative to the
  owning actor, while Parent names another component. Our runtime spatial components
  will store transforms relative to their immediate parent and inherit from it.
- Preserve the exported matrices and parent names. Do not silently reinterpret
  actor-relative data as parent-local. Agree on one conversion owner so conversion
  is performed exactly once; recommendation: engine-side build adapter.
- Use exactly 16 finite values per matrix, with explicit row ordering. A fixed-size
  array or a clearly documented matrix type is preferable to vector<float>.
- Confirm actor transform space, axis orientation, units, matrix/vector conventions
  and whether the importer already performs coordinate conversion. Row-ordered
  storage alone does not establish every mathematical convention.
- The supplied PDF references a Space Conversion section that is not included.
  Obtain those details or inspect the exporter implementation.
- Under the engine's current row-vector composition convention, after conversion:
  childParentLocal = childActorRelative * inverse(parentActorRelative).
  For parentless components, actor-relative is already local to the actor.
  Validate this against exporter fixtures before treating it as the final adapter.
- Singular parent matrices cannot be inverted. Agree on rejection behavior and a
  policy for negative/non-uniform scale and shear rather than silently losing data
  when converting a matrix to translation/rotation/scale.

## 2. Preserve concrete component data

- vector<ComponentData> cannot retain derived StaticMeshComponentData or light
  fields when objects are inserted by value: it slices them to the base type.
- Agree on variant storage of concrete descriptions, a tagged payload, or owning
  polymorphic pointers. Recommendation for the current typed model: a value variant
  with shared base metadata in each description.
- TypeID must represent -1, documented for custom serialization. Use a signed type
  or explicit enum representation rather than unsigned.
- Document the supported TypeIDs and custom type identity/payload. The engine does
  not need to implement every exporter type immediately, but neither side should
  silently discard unsupported components.
- Define complete types in dependency order. In particular, define the texture
  parameter payload before instantiating the material parameter variant.

## 3. Material representation

- Add a scalar alternative to MaterialParameterValue. The sample contains Type 0
  numeric values, Type 1 color arrays and Type 3 texture objects. Numeric parsing
  should accept both integer-looking and fractional JSON numbers for scalar values.
- Make MaterialData independent of ComponentData. Materials have no transform
  attachment, and repeated name/parent members introduce separate, hidden fields.
- Keep any material-parent reference distinct from a component-parent reference.
  Confirm whether that material field is actually supplied by the exporter version.
- Preserve material-array order because it identifies mesh material slots.
- Preserve texture/content identifiers as supplied. Unreal paths such as
  /Engine/BasicShapes/Plane.Plane are not directly usable filesystem paths.

## 4. Identity and extension data

- Actor names are documented as unique within a level; component names within an
  actor. Confirm Parent resolution scope and validate duplicates and unresolved
  references. The sample has no nested components or actor parent field.
- The builder can map import identities to runtime handles; names are not guaranteed
  stable across renames and should not be presented as persistent GUIDs.
- Preserve Archetype and tags. Archetype is the Unreal source class name, not an
  instruction to execute Blueprint behavior or an agreed engine prefab format.
- Clarify how custom component settings are carried for TypeID -1. Do not invent
  arbitrary tag/property interpretation independently on both sides.

## 5. Errors and validation fixtures

- Return structured import errors with file, actor/component and field context.
  A formatted boolean alone cannot explain failures or distinguish valid empty data.
- Parsing errors belong to the importer; registry/dependency/hierarchy validation
  belongs to the builder. Share checks where useful, but define who owns each one.
- The engine rejects invalid scenes before activation. Its load orchestration
  reports structured errors and preserves the old scene in Debug and Release.
  An initial requested load failure returns nonzero after partial cleanup.
- Request a fixture with a translated/rotated/scaled actor and at least three nested
  spatial components with non-identity offsets, plus expected world matrices.
- Also test two same-type components with different names, multiple material slots,
  scalar/color/texture parameters, custom/unsupported types, missing/cyclic parents,
  duplicate names, malformed matrices and zero/non-uniform/negative scale.
- Confirm light intensity conversion separately. The guide specifies attenuation
  radius in centimeters and spotlight angles in degrees, but omits the referenced
  intensity conversion ratio.

The existing 16-actor sample only exercises one parentless component per actor.
It is useful for basic parsing but cannot verify nested attachment conversion.
