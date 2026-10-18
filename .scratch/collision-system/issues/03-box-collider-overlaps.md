# 03: Add box collider overlaps

Status: resolved
Blocked by: 02

## Parent and implementation gate

[Collision detection MVP spec](../spec.md), criterion T5 and box coverage of T2-T4. Read [workflow](../workflow.md): source implementation is authorized by the resume request; see workflow.md.

## What to build

Extend the same code-spawned ColliderComponent to boxes, so a sphere entering a box or two boxes overlapping generates the same EventBus phases as sphere pairs.

## Acceptance criteria

- [x] SetBoxHalfExtents accepts finite positive dimensions, selects Box, and preserves prior state on invalid input.
- [x] Sphere/box and box/box use actual yaw-oriented geometry in either pair order; an enclosing AABB does not establish overlap.
- [x] Honor Actor-relative position, yaw rotation, and positive nonuniform box scale without shear.
- [x] Pitch/roll, reflection, shear, nonfinite data, and unsupported sphere scale are explicit diagnostics/exclusions, not silent approximations.
- [x] Touching counts as overlap; separation on Y prevents overlaps across floors.
- [x] Sphere and box Components use one shared phase/filter/lifecycle path, including shape changes while enabled.
- [x] Keep one collider per Actor and unchanged event payload/semantics.

## Verification

Public World/EventBus tests for sphere/box in both orders, box/box at varied yaw, corner/edge/face contact, rotated-box AABB false positives, vertical separation, combined offsets/scale, shape changes, invalid input, and phase transitions. Re-run sphere/lifecycle tests and public-header isolation. Register source membership where needed.

## Comments

No capsules, full 3D box rotation, sweeps, penetration response, import/tag schema, or general-purpose query interface.

## Verification evidence

Public World/EventBus/Collider tests verified in Debug. See evidence/03-red-build.log (box setter absent), evidence/03-boxbox-red-run.log (box touching initially absent), evidence/03-matrix-red-run.log (stale authored scale rejected valid raw matrix), and evidence/03-final-debug-build.log / evidence/03-final-debug-run.log (all sphere lifecycle and box scenarios pass). Coverage includes both sphere/box orders, box face/corner contact, varied yaw, parallel oriented-box AABB false positives, Y separation, composed offsets/nonuniform scale, shape changes, invalid dimensions, pitch/roll, raw shear/reflection/nonaffine/nonfinite/zero-scale matrices. The current source and composed matrices are authoritative, including SetMatrix overrides; their proper orthogonal bases supply positive scale lengths. Geometry uses double intermediates and a 1e-6 relative touching allowance for float matrix rounding. Collision-only public header isolation passes after final API: evidence/03-header-isolation.log. Whole-repo isolation remains unavailable due pre-existing failures captured in evidence/header-isolation-baseline.log. Root owns integrated Debug/Release regression builds and independent reviews. No staging or commits.
