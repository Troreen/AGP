# 01: Capture the real shape authoring and export contract

Status: ready-for-agent
Blocked by: none

## What to build

Produce reproducible box and sphere exports from the team's actual Unreal project,
with expected AGP geometry documented so the importer implementation has an
observable target. Spec: ../spec.md.

## Acceptance criteria

- [ ] Locate the Unreal workspace and exporter version without assuming the AGP
  client includes the Unreal project; record its sync state separately.
- [ ] Capture representative offset/yawed/scaled boxes and uniform spheres with
  disabled and multiple-shape examples. Record authored dimensions and transforms.
- [ ] Establish Bounds half/full size, unit/axis conversion, scale ownership,
  parent flattening, enabled/collision participation, and supported metadata.
- [ ] Decide how designers author layer/mask filtering, and document safe handling
  of multiple shapes and unsupported capsules without silently changing geometry.
- [ ] Record sanitized fixture locations, export procedure, expected geometry,
  and any decisions requiring the user's input. Update the spec's readiness.

## Verification

Inspect actual exporter implementation and exported values side by side with
Unreal authored values. Confirm transformed bounds in Game during ticket 02;
do not claim an editor export check from synthetic fixtures alone.
