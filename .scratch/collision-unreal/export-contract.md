# Unreal collision export contract investigation

## Confirmed environment

Unreal 5.7.4 project: the separate AGP-Content workspace, under
Unreal/P5G4_KnightHeart. Exporter source is in
Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp.
Unreal MCP 0.6.1 runs inside the editor at 127.0.0.1:18777/mcp. It was started
using the already-installed external plugin; health reports game-thread execution
and Python available. No MCP configuration or project plugin settings were changed.

The user selected /Game/Levels/Test_Levels/SP/Lvl_Component_Test. It is open and
already dirty; preserve existing changes and do not reload it. Current Actors
include SE_WorkflowTest, SE_TestObject, SE_Start, SE_End, SE_Spinner, a plane,
camera and directional light. The user subsequently supplied a fresh scene export
containing the TriggerBox component described below; do not assume the earlier
live-editor shape inventory is current.

## Actual supplied box export

Content/ExportedScenes/lvl_component_test/Lvl_Component_Test_Level.json contains
SE_WorkflowTest/TriggerBox, TypeID 6, Parent DefaultSceneRoot, and empty Tags.
Bounds is the object {x:32,y:32,z:32}. The component has uniform scale approximately
8.765 and UE-relative position (0,0,254). Actor UE position is (-1560,-130,0),
with identity rotation/scale. Under the existing axis mapping and unit factor,
expected engine center is (-130,254,-1560) and half extents approximately
(280.48,280.48,280.48). This provides a real fixture for scale-applied-once and
root-child placement checks. The scene does not yet contain a
CollisionScriptTriggerComponent record.

Accepted layer authoring: use component Tags already
supported by the exporter, e.g. CollisionLayer=Trigger and CollisionMask=Player.
Resolve a small fixed named layer map in game policy (Player=1, Trigger=2, Enemy=3), while
keeping ColliderComponent's numeric layer/mask API. Missing tags keep existing
defaults; malformed/unknown/duplicate settings receive clear diagnostics. This
adds no exporter fields and no native Unreal collision-profile behavior.

## Geometry and transforms

- Box serializer (HandleBoxComponent) emits Bounds as an object with lowercase
  x/y/z, via JsonObjectConverter. Its values are GetLocalBounds().BoxExtent:
  normally unscaled half extents. AGP currently incorrectly expects an array.
- Leave the exporter implementation unchanged. Verify the existing bounds and
  scale behavior against a real component-test-scene export.
- Sphere Radius is GetUnscaledSphereRadius. Capsule Radius/HalfHeight are also
  unscaled, but capsule detection remains outside this increment.
- Component Transform is a row-major relative matrix including scale. Root
  component relative transform is replaced with identity; Actor transform carries
  root placement. Parent is the attached component's name, or empty for a root.
- Existing AGP conversion maps UE (Y,Z,X) to engine (X,Y,Z), unit factor 1.
  Runtime dimensions must receive transform scale exactly once.
- Builder retains Parent as SourceParent metadata without composing nested
  component transforms. User chose to reject nested supported shapes and multiple
  supported shapes on one Actor for this increment.

## Collision settings

The user clarified that the existing export contract is the boundary and exporter
changes are not preferred. This supersedes the earlier proposed native-settings
extension. Do not modify the exporter or interpret Unreal Collision Presets,
GenerateOverlapEvents, channel responses or physics settings as exported data.

The programmer guide at Docs/Unreal Level Exporter_Usage for Programmers _ TGA
Wiki.pdf confirms TypeIDs 6/7/8 and shape-specific Bounds, Radius and HalfHeight.
Each component also has Name, TypeID, Parent, Transform and Tags. The guide permits
project-specific properties in Tags, but no tag parser is required by this scope.

Implemented: missing tags retain enabled=true, unnamed layer 0, and all mask bits.
Shape component tags CollisionLayer=<name> and CollisionMask=<name>|<name>
configure filters; an explicitly empty CollisionMask= accepts none. Exact names
are Player, Trigger and Enemy. The game injects their numeric mapping into the
importer; the engine does not own gameplay names. Malformed/unknown/duplicate
setting tags fail import with structured diagnostics. Tags remain source metadata.
No native Unreal collision-settings mapping is implemented.

## Verification limits and export access

The supplied scene now establishes a real box record from the unchanged exporter;
sphere export evidence and the AGP runtime round trip remain pending. Native TGA
export currently only exposes a C++ menu command
and directory picker, with no reflected Python-callable export entrypoint.
Do not label Python-reconstructed JSON as a genuine TGA export. Establish an
actual exporter invocation for the MCP verification before claiming that gate.

No exporter changes may be introduced solely to automate this verification.
