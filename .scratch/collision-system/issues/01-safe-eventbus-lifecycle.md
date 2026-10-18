# 01: Integrate the existing EventBus unchanged

Status: resolved
Blocked by: none

## Parent and revised scope

[Collision spec](../spec.md), criterion T1. The user explicitly requested that the teammate-owned EventBus remain unchanged. The former subscription-safety enhancement is superseded; its original ticket and tests are retained under ../superseded/eventbus/ as historical material only.

## Acceptance criteria

- [x] EventBus.hpp and EventBus.cpp have no Git content diff from HEAD.
- [x] CollisionSystem publishes through the existing Dispatch API.
- [x] Collision-owned Components/tests store std::uint32_t subscription tokens without exposing EventBus::ListenerID.
- [x] Gameplay subscribes in BeginPlay and unsubscribes in EndPlay using the existing methods.
- [x] Active documentation no longer promises the superseded subscription-mutation behavior.
- [x] Affected Debug builds and collision/lifecycle regressions pass with the restored EventBus.

## Verification

Compare both EventBus files with HEAD. Build the Debug solution and CollisionTests against the restored implementation; exercise existing subscription setup and EndPlay cleanup. Do not run the archived enhancement tests as current acceptance criteria. No staging or commits.

## Verification evidence

Restoration verified: both EventBus files have no content diff from HEAD. Debug solution build, CollisionTests and GameApplicationTests build/run pass with the original EventBus; logs are evidence/eventbus-unchanged-solution-debug.log, evidence/eventbus-unchanged-CollisionTests-debug-build.log / -run.log, and evidence/eventbus-unchanged-GameApplicationTests-debug-build.log / -run.log. Independent Spec/Standards reviewers found no restoration/glue defects. The local demo artifact is rebuilt against this original implementation and the main source/default is restored false afterward. Earlier enhancement-only test passes are historical, not acceptance evidence for the restored implementation.
