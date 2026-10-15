---
name: implement-spec
description: "Implement the result of /to-spec and /to-tickets in code."
---

You have been provided a spec. This spec should have tickets associated with it, describing how to implement the spec.

The issue tracker should have been provided to you. If not, tell the user to run `/setup-matt-pocock-skills`.

The goal is the entire spec implemented on a single **integration branch**, with every ticket resolved the way the issue tracker closes work. Follow `Docs/AgentWorkflow.md`: branch names use the feature slug without a `codex/` prefix or Codex suffix. Leave code and records uncommitted and unstaged unless the user explicitly requests otherwise. Pass these constraints to every subagent; never create commits merely to enable worktree integration or a PR.

The tickets are not a list of steps. They are a **task graph** with blocking relationships between them. This means there is always a **frontier** of tickets which are ready to be grabbed.

Communication to and from subagents should be sparse. Communicate primarily through **context pointers**: to the spec, tickets, research notes, and previous commits. Don't duplicate information already available via pointers.

**Implementer subagents** should be run in the background where possible for maximum concurrency.

## Steps

1. Read the spec and tickets to understand the task graph.

2. (optional) Use an **exploration subagent** to conduct any exploration required by the tickets - relevant codebase files or external documentation. Ensure the exploration subagent can save files - it should save its markdown notes in a directory outside the repo, accessible by all future subagents. This lets **implementer subagents** focus on implementation rather than exploration.

3. Create or reuse the feature-named integration branch. Open a PR only when explicitly requested and when user-authorized commits exist to publish.

4. Use **implementer subagents** for ready tickets, with disjoint file ownership in the feature checkout. Serialize overlapping edits. Isolated worktrees are optional when their dependencies are already available. Each implementer:
   - confirms the correct starting state without resetting away existing work;
   - reads the repo-local `tdd` skill and builds the ticket;
   - reports scoped uncommitted changes and verification evidence, without staging or committing automatically.

5. Integrate each implementer's reviewed changes in the feature checkout. Shared-checkout edits are already present; apply scoped diffs from isolated worktrees when necessary. Do not create commits or commit-producing merges to transfer work unless explicitly requested by the user.

6. If this changes the **frontier** of available tickets, kick off more **implementer subagents** to work on the new tickets. This allows for maximum concurrency.

7. Once all tickets are complete, call the Skill tool with `code-review` on the integration branch. Fix all issues raised by the code review in a single **implementer subagent**.

8. If a draft PR exists, mark it ready for review. Otherwise, resolve each ticket the way the issue tracker closes work, and report the integration branch.

9. Retain any worktree containing uncommitted or unfinished work. Clean up only managed worktrees whose changes have been safely integrated and preserved. Report the uncommitted result and remaining human checks.
