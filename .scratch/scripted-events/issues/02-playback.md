# 02: Play animations and registered audio
Status: in-progress
Blocked by: 01

Spec: ../spec.md

- [ ] Direct clips override normal animation; release restarts the tree's current clip without state entry/exit events, or holds the scripted pose without a tree. Tree bool/trigger action works.
- [ ] Audio actions start/check/stop the existing registered playback by SoundID, including positional playback.
- [ ] Non-wait actions overlap later steps; temporary playback cleanup is safe.

## Verification
Game build and CPU animation regression tests pass; independent review passes. Audio simplified after user review to one registered playback per SoundID; no independent instance pool. Audible/visual runtime check pending; see ../verification.md.
