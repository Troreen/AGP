# AGP review standards

Use these during the standards review. `.clang-format` and `.editorconfig` carry mechanical formatting rules; focus review on behavior and judgment.

- Prefer direct C++ and visible data flow consistent with nearby code. Introduce an abstraction when it solves the requested problem, and explain the tradeoff.
- Make data flow visible at the call site. Calculation and conversion functions should take read-only inputs and return their results, rather than silently modifying caller-owned vectors or values through output parameters. Group related outputs in a named result struct. Functions whose purpose is mutation should make the target and action clear in their names and interfaces.
- When a computed or constructed value may be unavailable and callers do not need a failure reason, return `std::optional<T>`; use a name such as `TryBuildWorldColliderShape` to make possible failure clear. Keep `bool` for yes/no questions such as `SphereOverlapsBox`. When callers need failure details, return a result that carries them.
- Use descriptive variable and parameter names that say what a value represents. Avoid vague names and unexplained abbreviations, especially in calculations; distinguish positions, distances, directions, indices, and current versus previous state.
- Give code room to breathe: use blank lines between logical steps and separate declarations for values with different roles. Follow `.clang-format` and `.editorconfig` for indentation and spacing.
- Use braces for every control-flow body, including single statements. Put opening and closing braces on their own lines, keep all brackets and parentheses correctly paired, and verify that edited C++ compiles.
- Keep gameplay in the application and Components; respect the existing engine interfaces and ownership described in the framework guides.
- Check lifetime and callback behavior across scene replacement and destruction. Borrowed Actor and Component pointers must not outlive their owners.
- Keep rendering, gameplay, input, and asset-loading changes within the agreed scope. Preserve earlier assignment behavior unless the spec changes it.
- Write everyday-language comments above logical sections to explain the higher-level purpose, how the steps fit together, and non-obvious choices. Help someone new to the code follow the flow without narrating every statement or merely restating the code.
- Review observable behavior, including boundary cases, rather than treating compilation as proof that a feature works. Rendering changes need visual evidence; performance claims need measurements.
- Flag speculative hooks, unnecessary dependencies, and broad refactors when a smaller change would satisfy the spec. Code smells are prompts for judgment, not automatic failures.
