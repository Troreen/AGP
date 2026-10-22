# Complete splatmap investigation plan

Latest continuation, 2026-10-22: owner requires preserving intentional yaw -90
and removing only the unwanted UV V flip. The individually approved scratch
correction/import test passed with -90 yaw; see
[measurements](audit-uv-corrected-yaw-minus90.json) and
[procedure/result](asset-correction-procedure.md). Earlier zero-yaw recommendations
are superseded. Production edits, visual scene changes and the separate DDS
experiment remain unauthorized. Prior investigations below retain their history.

Status: in-progress. Requested 2026-10-22. Workflow: agp-feature, research stage,
with grill-with-docs (grilling plus domain-modeling). No implementation authorized.

## Scope and stop condition

Investigate the actual MM_Landscape and its complete connected functions, debug
instance, painting data, geometry, exports and AGP consumption. Reconcile current
facts with historical notes. Iterate until every listed question has evidence,
a documented tool/access limitation, or an explicit owner decision pending.
"All information possible" means relevant actionable evidence, not an unbounded
scan of unrelated project assets. Ask before new experiments beyond the already
approved temporary weightmap diagnostic. No code/source-asset changes, saves,
staging, commits, content deployment, Perforce reconciliation or submission.

Owner clarification: MI_Landscape_SplatMapTesting deliberately substitutes pure
RGB debug colors and zero alpha. Distinguish this intended test representation
from the surface material and from the render_weightmaps export's unused alpha.

## Work graph and ownership

| Track | Owner | Questions and output | Prerequisites |
| --- | --- | --- | --- |
| 1. Live material and scene | root | Full master/function graph; ORM pin masks; normal convention; debug path; layer blending; Landscape/static-mesh relationships; durable raw evidence. `material-graph-audit.md` | Live MCP |
| 2. Unreal semantics | unreal_semantics agent | Alpha/weight blend formulas, order/defaults, Landscape UVs/weights, render_weightmaps registration and channel/linear-data behavior. `unreal-semantics-research.md` | Primary UE source/docs |
| 3. Runtime contract | runtime_contract agent | Export/import parameter transport, shader/resource limits, coordinates and projected normals, linear DDS/noise route, smallest candidate contract and validation matrix. `runtime-contract-research.md` | Local source/guides |
| 4. Assets and geometry | assets_geometry agent | Exact surface/normal/noise headers/settings available offline, FBX transform/UV metadata, duplicate asset roots, deployment/export provenance and missing files. `assets-geometry-research.md` | Local source/files |
| 5. Reconcile and question | root | Verify conflicts, dispatch targeted follow-ups, ask owner-only behavior questions with recommendations; update plan, workflow, terminology | Tracks 1-4 |
| 6. Independent evidence audit | available agents | Challenge integrated conclusions for unsupported claims and missing relevant checks; record residual gaps | First synthesis |

Each agent owns only its assigned note; shared Unreal editor access is serialized
through root. Agents may inspect files/read official docs; no editor mutations.
No additional agents beyond these named assignments without coordinating root.

## Verification checklist

- [ ] Capture actual project/map/material paths and current overrides.
- [ ] Traverse complete connected master/function graph, output masks/constants.
- [ ] Confirm debug interpretation and distinguish alpha concepts.
- [ ] Establish alpha blend order, fallback and overlapping-weight semantics.
- [ ] Trace painted weights and candidate extraction precision/registration.
- [ ] Establish axes, geometry units/bounds/UVs and intended 8m world mapping.
- [ ] Establish nine surface texture packing, color space and normal convention.
- [ ] Inventory noise candidates and separate source facts from design choices.
- [ ] Trace JSON/FBX/DDS/material lookup and resource budgets end to end.
- [ ] Identify minimal missing transport versus existing capabilities.
- [ ] Define rendering acceptance cases and measurements, without implementation.
- [ ] Ask unresolved owner decisions using grill-with-docs, record answers.
- [ ] Audit synthesis; record every remaining limitation explicitly.

## Rounds completed and new evidence

Round 1 ran all four tracks concurrently. Reports: [material graph](material-graph-audit.md),
[Unreal semantics](unreal-semantics-research.md), [runtime contract](runtime-contract-research.md),
[assets/geometry](assets-geometry-research.md).

Round 2 independently cross-checked evidence: [material review](material-evidence-review.md),
[assets review](assets-evidence-review.md), [runtime review](runtime-evidence-review.md).
It found stale limitations and line citations, not a substantive graph/binding
contradiction. Root is correcting the research records. Live mesh UV sampling
closed the Unreal-side orientation gap (first instance of all 4096 vertices).

Round 3 follows the owner's export/import question: [UV orientation cause](uv-orientation-cause.md).
Stored import Yaw=-90 is an actual non-default setting; Landscape FBX UV export
and mesh reimport have a separate V-convention mismatch. An owner-approved
temporary zero-rotation import confirmed both, preserving original assets.
Evidence: [temporary import results](audit-zero-rotation-import.json).
Correcting asset preparation is preferred to an HLSL compensation. A corrected
scratch FBX test and a linear DDS round-trip were separately requested and must
wait for their respective answers. No blanket asset-edit permission is inferred.

## Owner decisions recorded during grill-with-docs

- Preserve Dunes -> Rough -> Cliff overlay order.
- Use linear ORM; current Unreal sRGB ORM settings are not the parity target.
- Fill uncovered contribution with Dunes; this intentionally replaces black gaps.
  Algebraically this makes Dunes the continuous base under Rough/Cliff overlays.
- Noise perturbs existing transitions, preserves 0 and 1 weights, and cannot
  introduce a previously absent layer; strength zero reproduces baseline.
- Match original Landscape texture placement, not the converted mesh's current
  rotated UV orientation. World projection still repeats every 8 metres and is
  shared by terrain and ground floaters.
- Diagnose/correct export/import asset preparation first; do not compensate for
  the current mesh's incorrect import orientation in shader code.

## Initial state for this pass

AGP branch splatmap-blending, HEAD 517a6d7137f6fee2741d0a109b46126cf792062f.
Existing tracked edits: Bin/Settings/ApplicationSettings.json (user),
Docs/SplatmapMVPPlan.md (earlier research). Existing untracked research:
current-export-research.md, unreal-live-investigation.md, raw diagnostic PNG and
opaque preview. Preserve all. No staged changes observed. Original feature base
02886bf09c023aade3085315e6fb13fde1104e19 remains recorded in workflow.md.
