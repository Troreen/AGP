# Custom shader inputs workflow

- Request: recommended first batch; individual material includes; Unreal-authored values; do not commit or stage.
- Initial base: 9af6f528fda18b505dca699168c7ea67d6627c22.
- Initial branch: world-space-snow. Initial staged/unstaged/untracked scope: clean.
- Integration branch: shader-functionality (renamed current branch at user request). Attached checkout only.
- Stage: implementation.
- Spec: spec.md. Tickets: issues/01-surface-inputs.md, issues/02-material-import.md, issues/03-shader-time.md.
- Owners: surface-inputs agent (01), material-import agent (02), root (03). Tickets independent; builds serialized by root.
- Verification pending: solution/test builds, runtime visual checks, independent standards/spec reviews.
- Next action: implement three independent slices, review integrated diff, retain unfinished runtime checks explicitly.
