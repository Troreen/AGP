# AGP agent workflow

This repository uses the stable engineering and productivity skills from [mattpocock/skills](https://github.com/mattpocock/skills), inspired by [the workflow video](https://www.youtube.com/watch?v=Q3uuE2adUT8). The editable repo-local copy is pinned to commit `49dd158d1076134a641b33efb035946536778336`; it does not update itself.

## Start using it

For one-prompt delivery, use `$agp-feature Add [feature]. Run the full workflow.`
The [AGP feature skill](../.agents/skills/agp-feature/SKILL.md) coordinates
clarification, spec, tickets, implementation, verification, review, scoped
commits, and retrospective suggestions. It continues between routine stages;
unresolved behavior decisions and required human checks still need answers.
Progress is saved in `.scratch/<feature>/workflow.md`. To continue later, use
`$agp-feature Resume .scratch/<feature>/workflow.md`.

This runs during an active Codex session. It does not schedule future runs.
It can use parallel implementers and independent reviewers where those tools
are available, and reports any execution or verification limitations.

The skills are available on the next turn. If they do not appear, restart Codex. In Codex, mention them with `$skill-name`; the video's slash names refer to the same skills. These are prompted stages, not a daemon that runs automatically.

| Stage | Example prompt | Result |
| --- | --- | --- |
| Clarify | `$grill-with-docs I want to add [feature] to AGP.` | Resolve behavior decisions; record agreed terms and ADRs when useful. |
| Specify | `$to-spec Turn our agreed decisions into a spec.` | `.scratch/<feature>/spec.md`, including acceptance criteria and test seams. |
| Slice | `$to-tickets .scratch/<feature>/spec.md` | Small end-to-end tickets with explicit blockers. |
| Build one | `$implement .scratch/<feature>/issues/01-<ticket>.md` | Implementation, appropriate verification, independent review, then commit. |
| Build all | `$implement-spec .scratch/<feature>/spec.md` | Work through ready tickets in isolated worktrees on one integration branch. |
| Review | `$code-review Review my current changes against .scratch/<feature>/spec.md; use HEAD as the base.` | Separate spec and standards reviews. |
| Learn | `$retro Review this session and suggest improvements.` | Proposed environment improvements for the user to select. |

Use a fresh chat for each ticket, passing its path; every ticket must carry enough context to continue. `$ask-matt` helps choose a stage, `$wait-what` re-explains an answer, and `$grill-me` interviews without the domain-documentation step. Small, clear fixes can be handled directly without creating a spec and ticket tree.

## AGP adaptations

- Read `Docs/agents/issue-tracker.md` and `Docs/agents/domain.md` when upstream skills refer to tracker or domain configuration. Use the existing `Docs` directory rather than introducing a second `docs` tree.
- Use PowerShell for Windows procedures. Translate shell examples to the actual environment rather than adding a Bash dependency.
- Resolve code questions from the repo before asking the user. Ask about behavior choices that materially affect the result; an explicit user decision takes precedence over default skill ceremony.
- Agree observable test seams. Use existing tests for logic and lifecycle behavior. For visual rendering, specify a scene, expected result, controls, and captures. Do not invent a headless renderer or tests that merely repeat implementation details.
- Pin the review base before implementation. Review the selected staged, unstaged, and newly added files, as well as commits since that base. The upstream committed-only comparison is insufficient for pre-commit review. Give both review agents the same scope and base, preserving unrelated user changes.
- Run independent spec and standards reviewers when the review skill is invoked. The implementer need not preload `CODING_STANDARDS.md`; the standards reviewer reads it. Distinguish confirmed defects from optional design suggestions, and verify findings before changing code.
- `$implement` includes a commit after successful verification and review, unless the user asks otherwise. Commit only that ticket's files. Setting up these skills does not itself create commits, publish PRs, or merge branches.
- `$implement-spec` explicitly requests parallel implementation. Use `codex/` branch names and isolated worktrees based on the integration branch. Inspect existing changes before changing branches; do not reset away user work. Merge ticket work through the integration branch and archive only managed worktrees created for that run when no longer needed.
- A retrospective proposes fixes; it does not apply its own recommendations automatically.

## Verification for code changes

Build with Visual Studio 2026's C++ workload, Windows SDK, and v145 toolset. `SetupWindows.bat` describes dependency setup; avoid rerunning generation when a task does not need it. Changes to source membership must account for the relevant Premake files and checked-in Visual Studio projects.

From the repository root, a solution build in PowerShell is:

```powershell
$agpMSBuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
& $agpMSBuild .\Game.sln /m /nologo /p:Configuration=Debug /p:Platform=x64
if ($LASTEXITCODE -ne 0) { throw 'AGP build failed.' }
```

Locate MSBuild for the installed Visual Studio edition if that path differs. Build existing test projects directly when relevant; do not assume they are included in the solution:

- `Tests/GameFramework/GameFrameworkTests.vcxproj`
- `Tests/GameFramework/GameApplicationTests.vcxproj`
- `Tests/GameFramework/CustomComponentTests.vcxproj`
- `Tests/EngineOptimisations/EngineOptimisationsTests.vcxproj`

For example:

```powershell
& $agpMSBuild .\Tests\GameFramework\GameFrameworkTests.vcxproj /m /nologo /p:Configuration=Debug /p:Platform=x64
if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
& .\Bin\Tests\Debug\GameFrameworkTests.exe
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
```

Check each project's output and runtime dependencies before running it. `Tests/GameFramework/RunPublicHeaderIsolation.ps1` is an additional check for public gameplay-header changes; inspect its current assumptions and treat failures as evidence to investigate.

Run `Bin/Debug/Game.exe` for required runtime checks and compare each acceptance criterion with the observed result. Report the checks actually run, their results, and any checks blocked by the environment. A successful build is not a visual or performance check. This workflow installation changes documentation and skills only, so it does not require rebuilding the game.

## Maintenance

The original license is in `.agents/mattpocock-skills-LICENSE`. Local changes: `Docs` path conventions, Codex-compatible invocation metadata, and working-tree review instructions. `agp-feature` is a locally authored orchestrator; preserve it separately when updating the upstream skills. When updating, fetch the new upstream version separately, compare it with this pinned copy, and preserve these adaptations. Some personal skills have the same names; select the AGP-local copy when following this guide.
