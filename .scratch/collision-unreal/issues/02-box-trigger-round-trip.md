# 02: Run an Unreal-authored box encounter in Game

Status: in-progress
Blocked by: 01

## What to build

Deliver one complete box encounter: author/export a supported box and trigger,
import it as a real collider, and start its sequence when the normal Player enters.
Spec: ../spec.md. Use the contract and fixtures from ticket 01.

## Acceptance criteria

- [x] Box import and runtime construction preserve confirmed size, offset,
  transform, name and common metadata. Retain agreed AGP defaults for fields
  absent from the export; leave the exporter unchanged.
- [ ] Code-created Player has an appropriately measured collider; no duplicate
  exported Player is required.
- [x] Registered ScriptedEventComponent receives TriggerCollider from a real
  export alongside the same Actor's collider/actions; no bridge is required.
- [x] Player entry starts a simple sequence, logs activation, disables only the
  trigger collider, and permits sequence completion. Unrelated Actors do not start it.
- [x] Initial overlap, rejected Play then re-entry, consumed-state behavior,
  unsupported data, and one-collider policy have meaningful regression coverage.
- [ ] Debug Game confirms placement, gray consumed outline, and repeatable F4
  reload behavior in the named exported scene. Record captures or exact human limits.

## Verification

Use real fixtures through import/BuildWorld/World updates plus affected existing
collision, scripted-event and custom-component suites. Build Debug Game and run
the runtime acceptance procedure. Run header isolation for changed public headers.
Do not resolve this ticket while required visual checks remain unobserved.

The import/tag portion is implemented and verified through the focused real-box
fixture and public overlap callbacks; see ../verification.md. User explicitly
deferred restoring the shader-disabled Player and sequence activation to the next
increment, so this full encounter ticket remains open.

The user subsequently authorized that next increment. Player is restored with an
explicit radius60/localY90 body sphere approximation. The user completed the real
TGA export and confirmed F2 completion, automatic first entry, gray consumption,
and F4 restoring green and activation. No captures or exact Unreal/Game overlay
comparison were supplied. Actual-export regression and existing initial-overlap,
retry, unsupported-data and single-collider tests pass; see ../verification.md.

The requested scripted-event path works. This broader ticket stays in-progress
only because the measured/body-fit criterion remains an approximation, and its
original contract dependency includes a still-open real sphere export. Do not
request repeat-on-entry work: the user accepts one-shot behavior. No desktop
automation, automatic commit or Perforce submission is authorized.

The subsequent user-requested refactor moves the subscription into the sequence
using TriggerCollider=TriggerBox, removes the bridge and exact Actor-name filter,
and preserves successful-start consumption. Real re-export and updated automated
regressions pass. Reopened the visual criterion pending the fresh human check;
earlier human results apply to the prior arrangement.
