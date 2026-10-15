# 03: Lock player commands and hand control back
Status: in-progress
Blocked by: 01

Spec: ../spec.md

- [ ] Lock clears movement and blocks new commands while camera following remains active.
- [ ] Scripted movement can move a locked player.
- [x] Finish/cancel releases sequence-owned locks and movement suppression.
- [x] Explicit component enabling can release enemy AI.

## Verification
Public control/cleanup and component handoff tests pass; independent review passes. Manual camera/input/runtime check remains pending; see ../verification.md.
