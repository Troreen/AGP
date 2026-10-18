# 01: Disable manual character spawning
Status: in-progress
Blocked by: none

Spec: ../spec.md

## What to build
Comment out the full Player, Enemy, and A_Debug blocks in Game::ConfigureWorld, preserving the code. Applies to all scenes.

## Acceptance criteria
- [x] Player, Enemy and A_Debug setup blocks preserved as comments.
- [x] Updated Debug application builds.
- [ ] Startup/F4 visual check confirms all three manual characters remain absent.

## Verification
Debug/Release build; startup and F4 reload show no manual actors.

## Comments
Owner: root / renderer and asset agents as assigned in workflow.md
