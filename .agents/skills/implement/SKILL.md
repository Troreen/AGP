---
name: implement
description: "Implement a piece of work based on a spec or set of tickets."
---

Implement the work described by the user in the spec or tickets.

If the user passes a ticket reference, fetch it from the issue tracker and state its title before starting. If the reference is ambiguous, ask.

Call the Skill tool with "tdd" where possible, at pre-agreed seams.

Run typechecking regularly, single test files regularly, and the full test suite once at the end.

Once done, call the Skill tool with "code-review" to review the work.

Leave your work uncommitted and unstaged, preserving any pre-existing staging. Commit only when the user explicitly requests a commit; invoking this skill or the full feature workflow is not commit authorization. Follow the feature-name branch naming rules in `Docs/AgentWorkflow.md`, without a `codex/` prefix or Codex suffix.
