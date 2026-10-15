---
name: agp-feature
description: Coordinate the full AGP feature workflow from an idea or existing local spec through tickets, implementation, verification, independent review, and retrospective suggestions. Use when the user requests the full workflow, automated feature delivery, or resuming an agp-feature run.
---

# AGP feature

Run the requested feature through the repo-local workflow without requiring the user to invoke each stage. This skill owns orchestration; the linked skills supply procedures and templates. Read them when their stage begins, using available tools rather than assuming a Skill tool exists. Resolve paths from the repository root or this skill folder.

## Scope and continuation

Read `AGENTS.md`, `Docs/AgentWorkflow.md`, and `Docs/agents/issue-tracker.md`. Inspect Git status, the current branch, and existing feature files before writing. Record the initial staged, unstaged, and untracked scope and distinguish pre-existing user hunks from new feature hunks. The request authorizes local planning, scoped implementation, verification, and independent review. It does not authorize commits or staging. Leave changes uncommitted and unstaged unless the user explicitly requests those actions. A full-workflow request, passing checks, or approval of a spec is not commit authorization. Apply this rule to subagents and linked skills as well. Remote publication and merging require their own explicit user authorization.

Continue through routine stage transitions. An existing instruction to ask for approval of a spec, test seam, or ticket breakdown is satisfied by the user's full-workflow request for choices within its scope. Ask only when an unresolved behavior decision materially changes the result, a required human check is unavailable, or a permission/tool limitation prevents progress. User constraints always control the workflow.

Keep progress in `.scratch/<feature-slug>/workflow.md`: request and constraints, current stage, spec/ticket paths, original Git base, integration branch, active worktree paths and ticket owners, decisions, verification evidence, and the next action. Update at stage transitions and before yielding. Store durable context as file pointers rather than copying the whole conversation.

For a resume request, read that file and current Git/ticket state first; reconcile unfinished work before dispatching anything. Preserve completed records. Resume after a human check from the pending verification step. If multiple runs match, ask which feature the user means. Keep initial uncommitted changes outside feature commits unless the user explicitly includes them.

## 1. Resolve the feature

Explore the relevant code and existing guides. Read [grilling](../grilling/SKILL.md) for decision-tree questioning and [domain-modeling](../domain-modeling/SKILL.md) when terminology or architecture decisions need recording.

Find repo facts yourself. Use concise grouped questions with recommended answers for unresolved behavior decisions, continuing independent exploration while awaiting replies. Once the request and answers settle scope, proceed without an additional confirmation round. Record meaningful implementation assumptions; a clearly specified feature needs no interview.

If a decision needs an experiment, use [prototype](../prototype/SKILL.md) or [research](../research/SKILL.md) as appropriate. Keep the experiment scoped to the decision, with a pointer and verdict in the feature records.

Completion: observable behavior and scope are settled, and no unanswered decision blocks implementation.

## 2. Write the spec and tickets

Read [to-spec](../to-spec/SKILL.md) and use its template to write `.scratch/<feature>/spec.md`. Choose existing public test seams appropriate to the behavior and record their coverage and limitations. Ask about seams only when the choice changes the requested interface or leaves a material ambiguity; otherwise choose and document them under the full-workflow authorization. For rendering, specify runtime scenes and visual acceptance checks as well as relevant automated checks.

Read [to-tickets](../to-tickets/SKILL.md) and write one file per ticket under `issues/`, each small enough for a fresh implementer context. Use canonical `Status:` and `Blocked by:` lines from the tracker doc. Include spec pointers, observable acceptance criteria, verification, and dependencies. Present a brief scope/progress update and continue; routine ticket sizing needs no approval round. Reuse an existing adequate spec or ticket set instead of duplicating it.

Completion: every acceptance criterion belongs to a ticket, dependencies resolve to real ticket files, and the graph has no cycles. Every unresolved ready ticket either has satisfied blockers or an identified pending blocker.

## 3. Implement the ready tickets

Read [implement-spec](../implement-spec/SKILL.md) as the task-graph procedure and [implement](../implement/SKILL.md) as the per-ticket procedure. This wrapper authorizes subagent implementation and review when tools support it. Apply the AGP adaptations in `Docs/AgentWorkflow.md` to these procedures.

Use the feature slug alone for the integration branch, for example `scripted-events`; do not add a `codex/` prefix or any Codex suffix. Reuse a suitable feature branch on resume. Any additional branch must also have a descriptive feature/ticket name without Codex branding. Prefer the attached checkout so uncommitted feature work stays available. Use disjoint file ownership for concurrent agents; serialize overlapping edits. Isolated worktrees may be used when their dependencies are already available, but never create commits just to transfer work between checkouts. Apply reviewed scoped diffs when needed, or work serially. Never reset, clean, or stash away user work to prepare a checkout. Check build/runtime dependencies in each checkout rather than assuming ignored binaries and local assets are present.

Assign each ready ticket once and record its owner before dispatch. Run disjoint ready tickets concurrently; serialize tickets that would contend for shared files or build outputs. Each implementer gets the ticket, spec, relevant code pointers, base, and verification instructions. Use [tdd](../tdd/SKILL.md) at the recorded seams for meaningful behavior changes; low-impact documentation/formatting work needs appropriate validation rather than invented tests.

Review and verify each ticket before integrating its scoped changes into the feature checkout. Leave verified code uncommitted and unstaged, preserving any pre-existing staging. Integrate uncommitted work through reviewed scoped diffs or shared-checkout edits; do not run commit-producing merges automatically. Reverify combined changes before resolving blockers. If the user explicitly requests a commit, inspect the intended diff and commit only authorized feature hunks; preserve unrelated work and staging. Track unfinished ticket changes and worktree pointers for continuation. A failed integration or build remains unfinished work. If subagent/worktree tools are unavailable, work serially and report that execution mode; retain the same verification requirements.

Completion: all ticket code is integrated, with required verification evidence. A ticket awaiting a human runtime check stays `in-progress`, with the exact pending check recorded. On resume, reuse existing uncommitted changes and any user-authorized commits; do not recreate or automatically commit the implementation. When a runtime check needs combined code, provisionally integrate it on the feature branch to perform that check, retaining `in-progress` until it passes and keeping dependent tickets blocked. Otherwise complete the ticket-local check before integration. Saved changes and any user-authorized commits are continuation artifacts, not evidence of completion.

## 4. Verify and review the integrated feature

Run the relevant solution/test builds and runtime checks from `Docs/AgentWorkflow.md` against the integrated result. Use acceptance criteria as the checklist. Record commands, outcomes, and evidence; distinguish tests run from tests unavailable. Human-only visual checks should state the executable/scene, steps, and expected observable result. Continue checks that do not depend on the missing evidence, then request the required input.

Read [code-review](../code-review/SKILL.md). Dispatch two fresh reviewers with the same original base and selected feature diff, including staged, unstaged, and selected new files: one checks the spec, the other checks `CODING_STANDARDS.md` and the review skill's smell baseline. Give neither reviewer the other's conclusions. Keep their findings separate.

Verify findings before fixing them. Correct confirmed defects within scope, rerun affected checks, and have the relevant reviewer reassess changed code. Suggestions outside scope go in the retrospective. If independent reviewers are unavailable, disclose the limitation and perform separate spec and standards passes; do not describe them as independent. Resolve tickets and the spec only when required acceptance checks pass and confirmed in-scope defects are addressed.

Completion: the integrated feature meets every required acceptance criterion with evidence, and all remaining review findings are explicitly classified. Required checks that remain unavailable prevent declaring the feature complete.

## 5. Finish

Leave feature code and records uncommitted and unstaged unless explicitly requested otherwise. Report the branch, uncommitted change scope, implemented behavior, checks and results, and outstanding human actions. Report commits only if the user explicitly requested and received them. Retain feature files for continuation. Archive only managed worktrees created for this run that are no longer needed, after preserving their work; retain unfinished worktrees and their pointers.

Read [retro](../retro/SKILL.md) and present only useful, evidence-backed improvement suggestions. Applying those suggestions is separate work for the user to select. Open a PR only when requested, using [pr](../pr/SKILL.md) and attaching any created PR to the chat.
