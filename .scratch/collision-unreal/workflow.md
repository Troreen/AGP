# Collision Unreal continuation

- Request: plan the next collision steps with agp-feature, remove the optional
  collision demo, and commit/push the reviewed result to GitHub.
- Original Git base: 519d779527b162c31131e292dd9ad007ecc747c2.
- Branch: collision-system; attached checkout, no worktrees.
- Initial Git state was clean; no unrelated local changes were included.
- Current stage: requested sequence-owned TriggerCollider refactor implemented;
  actual re-export validated, Debug Game and three affected suites pass. Fresh
  human entry/gray/F4 check pending. Exact Player body fit and real sphere export
  remain open. No feature code staged or committed automatically.
- Spec: [spec.md](spec.md).
- Tickets: [01](issues/01-confirm-export-contract.md),
  [02](issues/02-box-trigger-round-trip.md), [03](issues/03-sphere-parity.md).
- Verification and review evidence: [verification.md](verification.md).

## Completed cleanup

Removed CollisionDemoComponent and its Game spawning switch/helper, game project
and filter entries, test project entry, and obsolete demo instructions. Retained
reusable collision components, scoped callbacks, scripted triggers, and generic
Debug outlines. The pre-existing disabled audio listener in Game.cpp is unchanged.

Temporary Perforce payload copies, generated diffs, manifests, helper scripts and
logs were removed at the user's request. No Perforce files were opened or submitted.
The GitHub push is authorized; do not infer Perforce submission authorization.

## Next action

Await human entry/gray/F4 verification of the sequence-owned trigger. Future collision work can
capture an actual sphere export and confirm Player body fit separately. Keep the
exporter unchanged; do not submit to Perforce or use desktop automation. No
repeat-on-entry feature is requested.

## Perforce context at planning time

Workspace tarik.bergstrom_TGA-SP85_Mainline_884 mapped this checkout to
//MAL25-G4/Mainline plus shared Content. Sync preview was up to date at Mainline
change 503854; the Unreal project directory is outside this client mapping.
Perforce's synced revision state does not imply Git changes have been submitted.
Collision implementation files were still missing from the depot.

World.cpp was also opened by thomas.bezerra@thomas.bezerra_TGA-SP86_P5_Mainline.
Recheck depot state and review exact diffs before any future Perforce submission.
Perforce ignores tests/generated VS projects/README; Premake recursively includes
production sources. Preserve the unrelated audio behavior difference between Git
and Perforce rather than including it in a collision-only submission accidentally.

## Resumed implementation run

- User requested committing the existing Git diff before collision work, then
  implementing Unreal-authored shapes with grill-me for decisions. No Perforce
  submissions are authorized. Keep work in small reviewable increments.
- Captured all pre-existing navigation/assets/shader cleanup as commit
  077d2c947e6fda96d65c8f9c26710da22e92e9e0. Git was clean afterward. This is the
  original collision implementation review base for this run. Baseline Debug
  Game.sln build succeeded with v145; existing warnings remain.
- Repo-local grill-me delegates to grilling. The user confirmed boxes/spheres
  produce nonblocking overlaps, capsules/physics remain outside scope, and one
  supported shape per Actor with nested attachments rejected. The user requested
  extending the exporter with settings now. Native settings mapping is pending
  their answer before implementation.
- User selected the component test scene. Unreal MCP is running and inspected the
  current scene without discarding its dirty map state. No scene assets modified
  or saved by the agent so far. No Perforce operations in this resumed run.
- Current stage: exporter contract investigation and behavior decisions. No new
  collision production code written yet. See export-contract.md for evidence,
  live editor context, and limits. Continue with the pending native-settings
  mapping decision, then settle compatibility/export invocation and implement.

### Exporter scope correction

The user pointed to the exporter programmer guide PDF and clarified that we should
work within the existing exported data rather than extend the exporter. This
supersedes the preceding native-settings extension proposal. Read and visually
checked the shape table: Bounds for boxes, Radius for spheres/capsules, HalfHeight
for capsules; common fields Name, TypeID, Parent, Transform and Tags. Updated the
spec/tickets/contract notes accordingly. No exporter files have been modified.
Pending grill-me decision is now whether to use AGP defaults for absent
enabled/layer/mask settings, or add an explicit convention using existing Tags.

### Supplied real fixture and layer recommendation

User exported SE_WorkflowTest/TriggerBox into the component test JSON. The scene
file's new working-tree changes are user-authored and must be preserved. Inspected
actual Bounds object, parent, uniform scale and offset; expected geometry is
recorded in export-contract.md. Proposed named component tags for layers/masks
using existing exporter data. No layer-authoring choice has been confirmed yet;
no new production collision code or exporter changes have been written.

### Import and tags implementation

- User accepted continuing with only named Player, Trigger and Enemy layers, and
  confirmed the current increment stops at import/tags. Player creation remains
  disabled for shader testing; no restoration or sequence demo in this increment.
- Scope: existing box/sphere records to ColliderData/ColliderComponent, component
  tag names resolved through injected game layer mapping, missing tags preserve
  numeric defaults, strict diagnostics for malformed/duplicate settings/shapes and
  nested attachments. Preserve the user-authored scene export unchanged.
- Owners: root implements importer/data/builder/game mapping; collision_import_tests
  owns focused fixture/public import-to-World tests. collision_game_integration
  performed read-only Player geometry inspection; no gameplay edits requested.
- Review base remains 077d2c947e6fda96d65c8f9c26710da22e92e9e0. Existing scratch
  planning changes and the user scene export predate new production edits.
- Test seam: importer -> BuildWorld -> World.Update/Collider callbacks. TDD first
  demonstrates the real Bounds-object parse failure, then implements geometry,
  then tag and invalid-input cases. Build outputs are serialized between owners.

### Completed import-and-tags increment

- Production: new ColliderData holds owned shape/filter data; importer reads the
  actual Bounds object, validates supported geometry/layout, decodes component
  layer tags and preserves metadata. SceneWorldBuilder creates ColliderComponent.
  GameApplication supplies the three-name game policy map.
- Tests: real exported TriggerBox subset verifies scale/offset via callbacks;
  synthetic sphere and unequal-axis yawed box exercise import geometry. Added
  filtering, empty mask, invalid names/dimensions/layouts, capsule-placeholder and
  current full scene import checks. Tests distinguish synthetic and actual inputs.
- CustomComponentTests previously read the editable scene's old probe values.
  The user's new export changes SpinSpeed and omits probes. Preserve user scene;
  freeze only its original SpinComponent probe from base 077d2c9 in a test fixture
  and retain every existing assertion. Final custom-component suite passes.
- Debug Game, Release Game, CollisionTests, ScriptedEventsTests,
  CustomComponentTests and GameApplicationTests build/run checks pass. Isolated
  SceneData, UnrealSceneImporter and GameCollisionLayers headers compile in Debug
  and Release without backend imports. Historical Release failure did not recur.
- Independent Spec and Standards reviews finish with zero findings. Their one
  coverage concern was resolved with the unequal-extents yawed import test.
- Evidence: verification.md and local Intermediate/CollisionUnreal logs. No
  exporter files, Unreal assets, Player setup or settings modified by this increment;
  no Perforce operations or submission. New source/records remain unstaged.

### Player and trigger integration

- User requested the next implementation stage and then explicitly prohibited
  computer-use automation, asking to do visual tests themselves. Computer-use
  initialization/list-apps occurred before that instruction; no desktop inputs
  were sent. No further computer-use actions will be taken. Unreal MCP authoring
  was continued as communicated; no native menu/picker automation was performed.
- Restored the existing normal Player mesh/animation/movement/controller setup.
  Added sphere radius60, local offset(0,90,0), Player layer and Trigger mask. This
  is a body-only approximation, not a measured full-mesh bound.
- Current ApplicationSettings scene selects the component test export. Removed
  the temporary unconditional asset-name print and lvl_blockout_level.nav
  __debugbreak from AssetRegistry, which could interrupt normal startup.
- Unreal MCP saved a new Blueprint asset at
  /Game/CustomComponents/ScriptedEventComponent/CollisionScriptTriggerComponent,
  using the existing serializable CustomComponent interface. StringVariables has
  only EnteringActor=Player and other serialized maps are empty. Added its instance
  CollisionScriptTrigger on level Actor SE_WorkflowTest and set TriggerBox tags
  CollisionLayer=Trigger, CollisionMask=Player. Box geometry and existing F2,
  Replayable and action settings were preserved. The Actor Blueprint was not edited.
- External saved files under AGP-Content/Unreal/P5G4_KnightHeart:
  Content/CustomComponents/ScriptedEventComponent/CollisionScriptTriggerComponent.uasset
  (new), Content/Levels/Test_Levels/SP/Lvl_Component_Test.umap (saved). No dirty map
  packages remain. No exporter source or .uproject configuration was modified.
- Actual AGP JSON has not been regenerated or reconstructed. User must run native
  exporter after the manual F2 baseline. Root will inspect CustomType, EnteringActor
  and shape tags in the resulting file before claiming exported-trigger completion.
- Debug Game.sln build passed. Independent Spec and Standards reviewers found
  zero defects in Game.cpp, AssetRegistry startup cleanup and scene selection.
  Debug CollisionTests, ScriptedEventsTests, CustomComponentTests and
  GameApplicationTests all rebuilt and ran with exit0 after Player restoration.
- User confirmed F2 sequence completes with the current exported JSON. This is
  human evidence for the baseline sequence; do not claim collider activation yet.
  User has been asked to re-export the saved map. Await their export confirmation,
  then inspect the JSON and request collision-entry/gray-outline/F4 verification.

### Confirmed real-export box encounter

- User ran the normal TGA export and confirmed completion. Inspected the actual
  updated component-test JSON: TriggerBox retains geometry and Trigger/Player
  tags; CollisionScriptTriggerComponent_C exports EnteringActor=Player. Existing
  coordinator/actions remain present, including Replayable=true.
- Human evidence: F2 sequence completes; first Player entry automatically starts
  the sequence and turns the box gray; F4 restores green and allows another entry.
  User confirmed all three checks. Re-entry without reset does not restart the
  consumed trigger, as intended. No repeat-on-entry setting is needed.
- Added a frozen focused subset of that real export and regression through
  import, registered custom-component construction and World.Update. Enemy and
  distant Player do not activate it; Player entry starts the authored Wait action,
  disables the box, completes and remains consumed after re-entry despite the
  coordinator's Replayable=true. Debug ScriptedEventsTests build/run exit0.
- No full-mesh body-fit comparison, real sphere export or sphere visual check is
  claimed. Those broader checks remain open; current box functionality is verified.
- No staging, feature commit, desktop automation or Perforce submission performed.

### Sequence-owned collision activation refactor

- User requested replacing the redundant trigger component with an optional
  exported TriggerCollider string on ScriptedEventComponent. It names a collider
  on the same Actor; blank keeps manual-only activation. Missing/wrong-type names
  produce diagnostics. Collision admission uses masks, not Actor-name matching.
- Sequence owns/relinquishes the scoped Enter subscription. Successful collision
  Play disables the selected collider, preserving gray one-shot consumption;
  rejected Play leaves it available for a later entry. Manual input still works.
- Deleted CollisionScriptTriggerComponent source, registration and VS memberships.
  Updated existing public behavior tests for optional/invalid references, initial
  overlap, rejected-start retry, consumption, scene replacement and name-independent
  layer filtering. Debug Game and ScriptedEventsTests, CustomComponentTests,
  GameApplicationTests build/run pass. Both independent reviews have zero findings.
- Unreal MCP saved ScriptedEventComponent.uasset with the empty String default,
  and Lvl_Component_Test.umap with TriggerCollider=TriggerBox on the existing
  sequence. Removed obsolete trigger instance and deleted its unreferenced Blueprint.
  Geometry, tags, F2, actions and Replayable=true preserved. No dirty map remains.
- User completed a fresh normal export. Actual JSON contains TriggerCollider on
  the coordinator and no old trigger instance. Refreshed the focused fixture from
  those actual component records; ScriptedEventsTests passes against it. Human
  entry/gray/F4 verification requested; earlier results apply to the old arrangement.
- No computer UI, staging, commit or Perforce submission. External assets are
  outside this Git repository and remain for later deliberate Perforce handling.
