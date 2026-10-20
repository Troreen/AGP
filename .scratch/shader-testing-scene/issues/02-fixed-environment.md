# 02: Add fixed hillside environment
Status: in-progress
Blocked by: none

Spec: ../spec.md

## What to build
Offline HDR-to-DDS cube conversion with ordinary mips; shader-scene and EAP existing ambient/reflection approximation; atomic load and default reset.

## Acceptance criteria
- [x] Source HDRI preserved; repeatable offline DDS conversion documented.
- [x] HDR-capable six-face ordinary-mip cubemap generated and validated.
- [x] Fixed environment wired for named shader scene and EAP; other named paths reset the default.
- [x] Final named shader scene loads without environment-load failure.
- [ ] Runtime failed-load preservation and other-scene comparison verified.

## Verification
Inspect HDR cube/mips and build; verify failed loads and another scene preserve correct environment.

## Comments
Owner: root / renderer and asset agents as assigned in workflow.md
