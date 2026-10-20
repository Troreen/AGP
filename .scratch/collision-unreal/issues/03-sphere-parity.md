# 03: Support and verify Unreal-authored sphere volumes

Status: ready-for-agent
Blocked by: 02

## What to build

Extend the demonstrated box path to sphere volumes with the same authoring,
filtering, lifecycle and sequence behavior. Spec: ../spec.md.

## Acceptance criteria

- [ ] A real exported sphere becomes a functioning collider with confirmed radius,
  offset, enabled state, and uniform scale applied exactly once.
- [ ] Nonuniform/invalid sphere transforms and unsupported capsule shapes receive
  the agreed diagnostics and never silently become another functioning shape.
- [ ] Box/sphere and sphere/sphere entry activate the intended sequence; filtering,
  disablement, destruction and reload retain their existing behavior.
- [ ] Automated regressions pass and the named scene demonstrates sphere parity.
- [ ] Update the collision authoring handoff with the verified procedure, supported
  fields, limits, and exact check results. Report current Release build status.

## Verification

Run public import/build/update fixture coverage and affected regression suites,
Debug Game build and visual acceptance. Run relevant public-header isolation and
attempt Release verification; document remaining toolchain limits accurately.
