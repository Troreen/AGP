# Benchmark Methodology and Busy-Scene Plan

## Goal

Turn the current benchmark into a trustworthy, reusable comparison system that can answer two separate questions:

1. Does an engine change add overhead or introduce regressions in the small default ModelViewer scene?
2. Does the change scale better when culling, snapshots, shadows, and command recording have meaningful work to do?

The next agent should improve the measurement method before adding the busy scene. A more demanding scene is useful only after run ordering and aggregation stop introducing avoidable bias.

## Current Repository State

- Active branch: `engine-optimizations`.
- Shared asset-layout baseline on local `main`: `7e1538c Use executable-relative asset paths`.
- Benchmark harness commits on `engine-optimizations`:
  - `1050c2b Add repeatable engine benchmark harness`
  - `423aec5 Track Release FBX runtime dependency`
  - `638e4ad Fix clean historical benchmark runs`
  - `a1b8233 Use shared asset layout in benchmarks`
  - `b824296 Group benchmark runs by version`
- The current report generator groups each version into one visual column and uses a shared scale for its run traces.
- Six real result directories and `Benchmarks/results/report.html` currently exist as untracked files: three runs for `main` and three for `engine-optimizations`.
- Preserve those result files. Do not delete, overwrite, stage, or reinterpret them as ten-run evidence without an explicit decision from the user.
- Preserve unrelated user files and generated folders, including `HandIn` if it is present.

## Evidence Motivating This Plan

The current three-run comparison shows:

| Metric | `main` | `engine-optimizations` | Change |
|---|---:|---:|---:|
| Median average FPS | 371.68 | 377.41 | +1.54% |
| Median 1% low FPS | 191.04 | 237.77 | +24.46% |
| Median frame time | 2.690 ms | 2.650 ms | -1.52% |
| Median P95 frame time | 3.375 ms | 3.361 ms | -0.41% |
| Median P99 frame time | 3.941 ms | 3.809 ms | -3.35% |
| Median frame-time standard deviation | 0.617 ms | 0.388 ms | -37.12% |

Across 3,600 measured frames per version, `main` had 34 frames above 5 ms while `engine-optimizations` had 3. This suggests a meaningful consistency improvement, but only a modest throughput improvement.

The old execution order was block-based: all `main` runs completed before all optimized runs. That allows GPU clocks, temperature, power state, or background activity to bias the second version. Three runs are also too few for a useful confidence interval.

## Settled Decisions

These are requirements, not open design questions:

- Default to **10 repetitions per version**.
- Build each requested ref once, then run the already-built executables in an interleaved and balanced order.
- Use the **arithmetic mean as the primary displayed aggregate**, as requested by the user.
- Display a **95% confidence interval**, sample standard deviation, and coefficient of variation beside the mean.
- Keep the median as a secondary robustness check.
- Do not show individual run cards or individual run traces in the main report.
- Continue saving every raw `frames.csv` and `summary.json`; aggregation must never destroy the underlying evidence.
- Keep `default` and `busy` scenario results separate. Never combine them into one aggregate.
- Store scenario and execution-order metadata in every summary.
- Keep the benchmark dependency-free: PowerShell, C++, and Python standard library only.

## Phase 1: Correct the Measurement Method

Complete this phase before implementing the busy scene.

### 1. Change the default repetition count

Update both `Benchmarks/run.ps1` and `Benchmarks/compare.ps1` so `Repetitions` defaults to `10` instead of `3`.

Keep the parameter configurable for smoke tests. A verification command may still request one or two short repetitions, but a report intended as evidence should use ten.

### 2. Build once and interleave execution

The current compare flow prepares, builds, runs all repetitions, and removes one ref before moving to the next. Refactor it so it:

1. Resolves all requested refs.
2. Creates all temporary worktrees.
3. Applies the benchmark-only instrumentation where required.
4. Builds every ref once.
5. Runs one repetition at a time from the already-built worktrees.
6. Alternates which ref runs first in each round.
7. Generates the report once after all runs finish.
8. Removes all temporary worktrees in `finally` cleanup.

For two refs named A and B, use a balanced order:

```text
round 1: A, B
round 2: B, A
round 3: A, B
round 4: B, A
...
```

This is preferred over always running A first. The sequence is interleaved even though the boundary between two rounds can contain the same version twice.

Likely script changes:

- Add a `-BuildOnly` mode to `run.ps1`, or extract the existing build logic into a reusable script/function.
- Add an explicit run-index parameter so ten separate `-Repetitions 1 -NoBuild` invocations save runs `1` through `10`, rather than ten files all called run 1.
- Continue using `-NoReport` for each individual invocation and run `generate_report.py` once at the end.
- Keep all worktree targets exact and validated before cleanup.

### 3. Record comparison metadata

Add environment/config fields and persist them in `summary.json`:

- `scenario`: `default` or `busy`.
- `comparison_id`: one identifier shared by every run in the same comparison session.
- `execution_index`: absolute position in the interleaved sequence.
- `run_index`: repetition number for that version, 1 through 10.
- `requested_ref`: the ref supplied to `compare.ps1`.

Retain the existing commit, branch, harness, configuration, resolution, CPU, GPU, warmup, and sample metadata.

Do not aggregate runs from different `comparison_id` values. Older summaries without this field should be labeled as legacy and kept separate from new comparisons.

### 4. Implement correct aggregate statistics

For each `(comparison_id, scenario, label, commit, configuration, machine, adapter, resolution)` group, calculate across the per-run summaries:

- Arithmetic mean.
- Median.
- Sample standard deviation using `statistics.stdev`.
- Minimum and maximum.
- Coefficient of variation: `sample_stddev / mean * 100`.
- 95% confidence interval for the mean.

Use a two-sided Student's t interval because ten runs is a small sample:

```text
standard_error = sample_stddev / sqrt(n)
CI95 = mean +/- t_critical(df=n-1) * standard_error
```

Use a small built-in 95% t-critical lookup table for supported sample counts rather than adding SciPy. For ten runs, `df=9` and the two-sided 95% critical value is approximately `2.262`. If fewer than two runs exist, show the mean but mark the interval and variation as unavailable.

Apply this aggregation independently to:

- Average FPS.
- 1% low FPS.
- Mean frame time.
- P95 frame time.
- P99 frame time.
- Mean Present time.

### 5. Replace individual-run presentation

Remove the current run-card grid from the main report. Raw runs remain on disk but should not dominate the visual result.

For each scenario, show one side-by-side aggregate comparison with:

- One version column or mark per commit/ref.
- Mean as the primary numeric value.
- A whisker/error bar for the 95% confidence interval.
- Median as secondary text.
- Sample standard deviation or coefficient of variation as the consistency value.
- Number of runs.

At minimum, visualize these metrics:

1. Mean average FPS with 95% confidence interval.
2. Mean 1% low FPS with 95% confidence interval.
3. Mean P95 frame time with 95% confidence interval.

The report should answer at a glance:

- Which version is faster on average?
- Is the observed difference larger than run-to-run uncertainty?
- Which version is more consistent?

Do not hide uncertainty behind a single average. Do not discard or rewrite raw per-run logs.

### 6. Separate comparison sessions and scenarios

Do not mix the existing three-run data with the future ten-run data.

Preferred behavior:

- Group report sections by `comparison_id` and then by `scenario`.
- Clearly label summaries without a `comparison_id` as legacy.
- Optionally add a command-line `--comparison-id` filter to generate a focused report for one session.

The existing six result folders should remain readable by the generator.

## Phase 2: Add a Deterministic Busy Scenario

Begin only after Phase 1 passes its smoke tests.

### 1. Scenario selection

Add a benchmark scenario parameter:

```powershell
.\Benchmarks\compare.ps1 `
  -Refs main,engine-optimizations `
  -HarnessCommit <latest-harness-commit> `
  -Scenario busy `
  -Repetitions 10
```

Requirements:

- `-Scenario` validates `default` or `busy`.
- `run.ps1` passes the scenario through an environment variable such as `AGP_BENCHMARK_SCENARIO`.
- `FrameBenchmark` stores the scenario in `summary.json`.
- ModelViewer selects the deterministic scene from the same scenario value.
- Normal launches without the benchmark environment continue loading the existing default scene.

### 2. Keep scene construction identical on both versions

The busy scene must not exist only on `engine-optimizations`; that would make the comparison invalid.

Prefer a small benchmark-scene helper and a minimal hook that can be committed to `main` and then applied equivalently to `engine-optimizations`. The scene definition, transforms, mesh choices, materials, and light setup must be identical on both branches. Verify the relevant helper contents rather than assuming a conflict resolution preserved them.

Do not merge render optimizations into `main` while adding the scenario.

### 3. Busy-scene specification

Keep the workload understandable and deterministic:

- Start with the existing camera, floor, character, and three-light setup.
- Add approximately **256 static primitive mesh actors** in a fixed 16 by 16 layout.
- Reuse registered primitive meshes and cached existing materials.
- Use index-based deterministic transforms; do not use an unrecorded random seed.
- Place a meaningful portion inside the camera frustum and a meaningful portion outside it.
- Include visible and offscreen shadow casters.
- Reuse the existing directional, point, and spot lights for the first version of the scenario instead of adding many new variables at once.
- Avoid per-run asset generation, loading, camera motion, user input, or nondeterministic animation changes.

Before fixing the final actor count, verify that the scene runs reliably on the NVIDIA MX450 and remains short enough for 20 total runs. The target is a several-minute comparison, not an hour-long stress test.

The busy scenario should primarily exercise:

- Camera frustum culling.
- Relevant-light filtering.
- Per-cascade and per-light shadow caster culling.
- Snapshot construction and reuse.
- Resource prewarm.
- Deferred shadow command recording.
- Per-frame task scheduling overhead.

### 4. Keep the default scenario

Do not replace the existing scene.

- `default` remains the lightweight overhead/regression benchmark.
- `busy` becomes the scalability/culling/shadow benchmark.

A meaningful optimization should ideally avoid regressing `default` while improving `busy`.

## Phase 3: Verification

### Automated/statistical verification

Add focused standard-library tests for report aggregation. Synthetic fixtures should prove:

- Ten runs are grouped into one aggregate.
- Different commits, scenarios, machines, or comparison IDs are never merged.
- Mean, median, sample standard deviation, coefficient of variation, and 95% interval are correct.
- Legacy three-run summaries remain readable but separate.
- The generated report contains aggregate/error-bar presentation and no individual run-card section.

### Script verification

- Parse both PowerShell scripts to catch syntax errors.
- Run `git diff --check`.
- Confirm temporary worktrees are removed after success and after a forced failure.
- Confirm each ref is built once during a ten-run comparison.
- Confirm run indices and execution indices are unique and correct.
- Confirm the actual execution order alternates the first ref each round.

### Short smoke comparison

Before the full evidence run, use small settings such as:

```powershell
.\Benchmarks\compare.ps1 `
  -Refs main,engine-optimizations `
  -HarnessCommit <latest-harness-commit> `
  -Scenario default `
  -WarmupFrames 5 `
  -SampleFrames 10 `
  -Repetitions 2
```

Delete only these explicitly identified smoke-test outputs after verification. Never delete the user's six current real result folders.

### Full comparisons

After the smoke test passes, run in one stable session with unrelated heavy applications closed:

```powershell
.\Benchmarks\compare.ps1 `
  -Refs main,engine-optimizations `
  -HarnessCommit <latest-harness-commit> `
  -Scenario default `
  -WarmupFrames 300 `
  -SampleFrames 1200 `
  -Repetitions 10

.\Benchmarks\compare.ps1 `
  -Refs main,engine-optimizations `
  -HarnessCommit <latest-harness-commit> `
  -Scenario busy `
  -WarmupFrames 300 `
  -SampleFrames 1200 `
  -Repetitions 10
```

Do not claim a win merely because the optimized mean is better. Check whether confidence intervals, effect size, and variation support the conclusion.

## Interpretation Rules

Use these rules in the report and handoff notes:

- **Improved:** the effect is practically meaningful, repeatable, and larger than uncertainty.
- **Neutral:** the difference is small relative to the confidence interval or run variation.
- **Regressed:** the optimized result is meaningfully worse with uncertainty accounted for.
- Treat average FPS as throughput.
- Treat 1% low, P95/P99, and variation as consistency/tail behavior.
- Treat Present time as diagnostic evidence, not proof of reduced engine CPU work.
- The current harness does not measure GPU timestamps or total CPU use across worker threads.

## Follow-Up After the Busy Scene

If the busy scene still shows only a small throughput gain, do not immediately add more objects. Add scoped CPU timings for:

- Snapshot construction.
- Resource prewarm.
- Shadow caster filtering.
- Shadow job construction.
- Worker command recording and wait time.
- Deferred command-list playback.
- Main-scene command recording.
- Present.

The current code launches per-frame `std::async` shadow jobs. With the default scene this can create approximately six small tasks per rendered frame, so a persistent worker pool is a likely next optimization. Profile first; do not assume it is the bottleneck.

## Suggested Commit Boundaries

Keep the work reviewable:

1. `Interleave and aggregate benchmark repetitions`
   - Ten-run default.
   - Build-once/interleaved execution.
   - Comparison metadata.
   - Mean/CI aggregation and report redesign.
   - Tests.
2. `Add deterministic busy benchmark scene`
   - Scenario plumbing.
   - Identical shared scene setup on `main` and `engine-optimizations`.
   - Scenario-aware reports.
3. `Record default and busy benchmark evidence`
   - Only after the real runs are accepted.
   - Raw result directories and regenerated reports.
   - Short written conclusion.

Do not combine engine optimizations with benchmark methodology or scene-construction commits.

## Definition of Done

The handoff is complete when:

- Ten repetitions per version are the default.
- Refs build once and execute in balanced interleaved order.
- Every result stores scenario, comparison ID, execution index, and run index.
- Reports aggregate with mean, median, sample variation, and a Student's t 95% confidence interval.
- The main visual report contains no individual run cards.
- Legacy and new comparison sessions do not mix.
- `default` and `busy` scenarios are deterministic and separated.
- The busy scene is identical on both measured branches.
- Short smoke verification passes without leaving worktrees or smoke artifacts.
- Full ten-run default and busy comparisons complete successfully.
- No unrelated or user-owned files are modified, deleted, or committed.
