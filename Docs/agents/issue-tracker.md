# Issue tracker: local Markdown

Specs and tickets live in this repository. Publishing means writing a local file; no external issue tracker is involved.

- Feature directory: `.scratch/<feature-slug>/`.
- Spec: `spec.md`, with a `Status:` line.
- Tickets: `issues/<NN>-<slug>.md`, numbered from `01` in dependency order; one ticket per file.
- Each ticket records `Status:`, `Blocked by:` (ticket numbers, or `none`), acceptance criteria, and verification requirements.
- Default triage states are in [triage-labels.md](triage-labels.md). Set `Status: in-progress` when claiming implementation work and `Status: resolved` only after its acceptance criteria and required verification pass.
- A ticket is ready when it is `ready-for-agent` and every blocker is `resolved`.
- Record comments under `## Comments`, and completion evidence under `## Verification`.
- Resolve the spec only when every required ticket is resolved. Keep completed files as history.

When asked to fetch a ticket, read its referenced file. Resolve numbers within the supplied feature directory; ask when multiple features match.

For wayfinding, use `.scratch/<effort>/map.md` and one child file per question under `issues/`. Children record `Type: research/prototype/grilling/task`, `Blocked by:`, and `Status: open/claimed/resolved`. Append resolved answers to each child and a short linked decision to the map. Pick the lowest-numbered open, unclaimed child whose blockers are resolved.

Do not ignore `.scratch/`: local specs and tickets should be available in a fresh checkout once committed. No issue or feature is created merely by installing the workflow.
