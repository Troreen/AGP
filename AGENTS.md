# AGP agent navigation

This is a Windows C++20 / DirectX 11 project. Build `Game.sln` with Visual Studio's v145 toolset. Read [README.md](README.md) for setup and runtime paths.

- Game behavior and frame order: [Docs/GameLoopOnboarding.md](Docs/GameLoopOnboarding.md).
- Engine ownership and scene construction: [Docs/EngineArchitectureBasics.md](Docs/EngineArchitectureBasics.md).
- Framework interfaces: [Docs/GameFrameworkMVP.md](Docs/GameFrameworkMVP.md).
- Workflow and verification: [Docs/AgentWorkflow.md](Docs/AgentWorkflow.md). Follow its AGP adaptations when using the installed workflow skills.
- Reviewers: read [CODING_STANDARDS.md](CODING_STANDARDS.md) for judgment calls.

## Agent skills

The Matt Pocock workflow is installed locally in `.agents/skills/`. When a name also exists in personal skills, use the repo-local version for this workflow. When an instruction says to call a Skill tool, read the referenced repo-local `SKILL.md` and follow it using the tools available in this session.

For a full automated feature run or resume, use [agp-feature](.agents/skills/agp-feature/SKILL.md); it coordinates the stages and records continuation state.

### Issue tracker

Specs and tickets are local Markdown files under `.scratch/<feature>/`. See [Docs/agents/issue-tracker.md](Docs/agents/issue-tracker.md).

### Triage labels

Use the default role strings in [Docs/agents/triage-labels.md](Docs/agents/triage-labels.md).

### Domain docs

One root `GLOSSARY.md` and `Docs/adr/`, created when needed. See [Docs/agents/domain.md](Docs/agents/domain.md).
