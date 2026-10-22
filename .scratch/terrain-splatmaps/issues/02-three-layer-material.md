# 02: Three complete world-projected surface layers

Status: in-progress
Blocked by: 01
Owner: root

Spec: ../spec.md

Deliver the fixed three-layer opaque lit shader, texture noise and controls,
debug views and consistent slope-preserving world normal mapping.

Acceptance criteria:
- Dunes base, Rough then Cliff ordered overlays blend color/ORM/normals.
- Linear texture noise preserves endpoints/absent layers; strength zero is baseline.
- Terrain and rotated floaters share world placement and 8m surface repeats.
- Flat normal preserves slopes; zero-vector cancellation is safely handled.
- Compiled resources/data stay within the engine's caps.

Verification: real shader compilation plus worked-example GPU readbacks and
integrated runtime visual checks. Cannot resolve on compilation alone.
