# Independent collision system source review

Base: d627d712a1024b4cfb7f1b623692e51ef4714ae9. Scope: [review-scope.md](review-scope.md).

## Standards

Fresh standards reviewer: 0 confirmed documented-standard violations and 0 actionable optional smell findings. Reviewed ownership/lifetimes, geometry versus gameplay, EventBus callable identity, demo cleanup, support repairs, tests and documentation. This is a source-review pass; builds and runtime observation are separate gates.

## Spec

Fresh spec reviewer: 0 confirmed source-level missing requirements, incorrect collision behavior, or gameplay/combat scope creep. Public contracts, frame membership, filtering, identities, overlap phases, EventBus mutation safety, and opt-in neutral demo match the spec.

T6 runtime observation and final T7 integrated verification remain pending at source-review time. Legacy GameFrameworkTests and full-repository header isolation have recorded baseline failures. New-only header checks and the current custom-component regression substitution are recorded separately. See workflow.md and ticket 04 for final evidence.

Summary: Standards 0 findings; Spec 0 source findings. No code corrections requested.

Both reviewers reassessed the later macro-safe std::max spelling, DurationInSeconds fixture/readback key alignment, and removal of nonexistent EnemyComponent test membership; zero findings remain. Final verification status is recorded in verification.md.

The later user decision removes the EventBus enhancement from feature scope. Statements above about snapshot ownership refer to that earlier reviewed implementation only; no subscription-mutation guarantees are claimed for the restored EventBus. Current restoration/integration checks are in verification.md.

Independent reassessment of the ownership revision: both reviewers found no restoration/glue defects. EventBus has no content diff from HEAD, collision-owned tokens use std::uint32_t with explicit includes, and active docs do not promise the removed mutation semantics.
