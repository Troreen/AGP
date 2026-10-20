# 02: Run an Unreal-authored box encounter in Game

Status: ready-for-agent
Blocked by: 01

## What to build

Deliver one complete box encounter: author/export a supported box and trigger,
import it as a real collider, and start its sequence when the normal Player enters.
Spec: ../spec.md. Use the contract and fixtures from ticket 01.

## Acceptance criteria

- [ ] Box import and runtime construction preserve confirmed size, offset,
  transform, enabled state, name, and agreed filtering semantics.
- [ ] Code-created Player has an appropriately measured collider; no duplicate
  exported Player is required.
- [ ] Registered CollisionScriptTriggerComponent receives EnteringActor from a
  real export alongside the same Actor's coordinator/actions.
- [ ] Player entry starts a simple sequence, logs activation, disables only the
  trigger collider, and permits sequence completion. Unrelated Actors do not start it.
- [ ] Initial overlap, rejected Play then re-entry, consumed-state behavior,
  unsupported data, and one-collider policy have meaningful regression coverage.
- [ ] Debug Game confirms placement, gray consumed outline, and repeatable F4
  reload behavior in the named exported scene. Record captures or exact human limits.

## Verification

Use real fixtures through import/BuildWorld/World updates plus affected existing
collision, scripted-event and custom-component suites. Build Debug Game and run
the runtime acceptance procedure. Run header isolation for changed public headers.
Do not resolve this ticket while required visual checks remain unobserved.
