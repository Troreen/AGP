# 03: Scene simulation time for material shaders
Status: in-progress
Blocked by: none
Owner: root

Spec: ../spec.md

## Acceptance criteria
- World update accumulates sanitized delta; snapshot exposes stable elapsed seconds.
- Clear/new World resets clock; invalid delta cannot poison time.
- Camera and shadow material draws receive the same simulation time.

## Verification
Public World update/snapshot regression tests, Game.sln and GameFrameworkTests builds, animated material runtime check.
