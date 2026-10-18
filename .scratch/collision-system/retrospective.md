# Suggested environment improvements

These are proposals only; none were applied automatically.

- Add a deterministic fresh-build/project-membership check. Existing LineDrawer/shader/BehaviourTree sources were covered by Premake globs but absent from checked-in projects, which delayed verification and caused compile/link failures.
- Keep current regression targets compiling when public APIs change. DurationInSeconds, removed ServiceLocator setters and deleted source paths left old suites stale. A lightweight CI build of the test projects would expose this immediately.
- Repair the whole-repository header-isolation baseline separately, then keep it running on public-header changes. The collision-only pass deliberately does not conceal its existing failures.
