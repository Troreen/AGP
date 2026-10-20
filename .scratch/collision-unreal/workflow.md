# Collision Unreal continuation

- Request: plan the next collision steps with agp-feature, remove the optional
  collision demo, and commit/push the reviewed result to GitHub.
- Original Git base: 519d779527b162c31131e292dd9ad007ecc747c2.
- Branch: collision-system; attached checkout, no worktrees.
- Initial Git state was clean; no unrelated local changes were included.
- Current stage: planning and demo cleanup complete; Unreal collider import has
  not started. Spec remains needs-info until the exporter contract is confirmed.
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

Start ticket 01: locate the actual Unreal workspace/exporter and establish the
shape export contract using real box/sphere fixtures before implementing import.
Then deliver a complete box encounter with Player, followed by sphere parity.

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
