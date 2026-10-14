# AGP review standards

Use these during the standards review. `.clang-format` and `.editorconfig` carry mechanical formatting rules; focus review on behavior and judgment.

- Prefer direct C++ and visible data flow consistent with nearby code. Introduce an abstraction when it solves the requested problem, and explain the tradeoff.
- Keep gameplay in the application and Components; respect the existing engine interfaces and ownership described in the framework guides.
- Check lifetime and callback behavior across scene replacement and destruction. Borrowed Actor and Component pointers must not outlive their owners.
- Keep rendering, gameplay, input, and asset-loading changes within the agreed scope. Preserve earlier assignment behavior unless the spec changes it.
- Write comments that explain purpose or a non-obvious choice in plain language. Avoid restating the code.
- Review observable behavior, including boundary cases, rather than treating compilation as proof that a feature works. Rendering changes need visual evidence; performance claims need measurements.
- Flag speculative hooks, unnecessary dependencies, and broad refactors when a smaller change would satisfy the spec. Code smells are prompts for judgment, not automatic failures.
