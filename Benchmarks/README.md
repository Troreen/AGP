# AGP Engine Benchmarks

This benchmark answers one deliberately simple question:

> With the same ModelViewer scene, machine, resolution, and Release build, how much steady-state frame throughput changed between engine commits?

The harness measures CPU-observed frame intervals at the shared RHI `Present` boundary. That location works for both the old main-branch application loop and the optimized snapshot/threaded loop, so an optimization does not need benchmark-specific integration.

## What Each Run Saves

Every repetition creates a timestamped directory under `Benchmarks/results` containing:

- `summary.json`: commit, branch, build, hardware, resolution, run settings, average FPS, 1% low FPS, and frame/present percentiles.
- `frames.csv`: every measured frame interval and `Present` duration for spike inspection or external analysis.

`Benchmarks/results/report.html` is regenerated from all summaries. It separates comparison sessions and scenarios, then shows run-level arithmetic means with Student's t 95% confidence intervals, medians, sample standard deviations, and coefficients of variation. Raw runs stay on disk but are not presented as individual cards or traces. The report is self-contained and can be opened directly in any normal browser.

Results are intentionally not ignored by Git. Commit the raw results and regenerated report after a real comparison so the evidence stays beside the optimization history. Do not commit short smoke-test runs.

## Normal Run

Use a clean, committed checkout and close unrelated heavy programs:

```powershell
.\Benchmarks\run.ps1 `
  -Label "before-thread-pool" `
  -WarmupFrames 300 `
  -SampleFrames 1200 `
  -Repetitions 10 `
  -Notes "ModelViewer default scene"
```

The script:

1. Builds `ModelViewer` as `Release | x64`.
2. Links the freshly built shaders into the expected ignored runtime location.
3. Discards the warmup frames.
4. Saves the requested number of raw frame samples.
5. Closes ModelViewer automatically.
6. Repeats the run and regenerates `report.html`.

Ten repetitions are the default because a small desktop sample can be noisy. The report uses the arithmetic mean as the primary result and keeps the median as a robustness check. Standalone `run.ps1` invocations receive their own comparison ID; use `compare.ps1` for balanced cross-version evidence.

## Compare Existing Commits

Keep benchmark harness changes separate from engine optimizations. Pass the latest harness setup commit to `compare.ps1`; it traces back to the commit that introduced `FrameBenchmark.cpp` and extracts the complete instrumentation range. Older refs can then be measured with the exact same harness without rewriting this branch:

```powershell
.\Benchmarks\compare.ps1 `
  -Refs main,2f91e36,8c0efbb `
  -HarnessCommit <benchmark-harness-commit> `
  -WarmupFrames 300 `
  -SampleFrames 1200 `
  -Repetitions 10
```

For every ref, `compare.ps1` creates a detached temporary worktree and applies only the benchmark source/project instrumentation from the benchmark commit when necessary. It prepares every requested ref first, builds each ref exactly once, and then runs one repetition at a time in a rotating order. For two refs the order is `A,B`, then `B,A`, which balances which version starts each round. All worktrees remain available for the whole comparison and are removed in `finally` cleanup.

Every summary stores a shared `comparison_id`, `scenario`, absolute `execution_index`, per-version `run_index`, and `requested_ref`. The report never merges different comparison IDs, scenarios, commits, machines, adapters, configurations, or resolutions. Summaries created before these fields are labeled as legacy and stay separate.

`libfbxsdk.dll` is an essential runtime dependency and is tracked for both Debug and Release. Historical worktrees receive the Release DLL through the same temporary binary harness patch, so Windows can resolve it beside `ModelViewer.exe`.

For the current history, the useful first comparison is:

- `main`: unoptimized reference (`cb880ae` at the time this harness was created).
- `2f91e36`: render optimizations.
- `8c0efbb`: threaded input/asset-location follow-up.

Do not treat those hashes as permanent names; prefer named tags for important checkpoints.

## Workflow For The Next Optimization

1. Start from a clean committed state.
2. Run a clearly named `before-*` benchmark.
3. Make one optimization and commit it.
4. Run a matching `after-*` benchmark under the same conditions.
5. Open `Benchmarks/results/report.html` and inspect the mean, 95% interval, median, and variation for average FPS, 1% low, and P95 frame time.
6. Commit the result directories and report as performance evidence.
7. Record the conclusion: improved, neutral within noise, or regressed.

If the effect is small relative to the confidence interval or run-to-run variation, call it neutral. Do not claim an improvement based on one unusually good run.

## Reading The Metrics

- **Average FPS:** overall throughput. Higher is better.
- **Average frame time:** the same throughput expressed in milliseconds. Lower is better.
- **1% low FPS:** FPS derived from the slowest one percent of measured frames. Higher means fewer visible hitches.
- **P95/P99 frame time:** 95%/99% of frames completed at or below this cost. Lower is better.
- **Present time:** time spent inside DXGI `Present`. Use it as supporting diagnostic data, not as the optimization score.
- **95% confidence interval:** Student's t interval around the run-level arithmetic mean. With ten runs the two-sided critical value uses 9 degrees of freedom.
- **Sample standard deviation / coefficient of variation:** absolute and relative run-to-run variability. Lower variation means a more repeatable result.

The headline numbers are CPU-observed frame intervals, not Direct3D GPU timestamp queries. They include application work, command submission, and driver back-pressure. They are a good first benchmark for the current CPU/scheduling optimizations. Add GPU timestamps later if an optimization specifically targets shader or GPU cost.

## Fair-Comparison Rules

- Use `Release | x64`.
- Keep the ModelViewer window, default scene, camera, and light state unchanged.
- Use the same machine, power mode, GPU, resolution, warmup, and sample count.
- Run versions in one comparison session so execution can be interleaved and balanced.
- Avoid interacting with the benchmark window.
- Reject dirty-source runs unless the only difference is the explicitly recorded temporary harness.
- Look at repetition spread and traces before trusting a percentage.

The JSON summary stores configuration, CPU identifier, GPU adapter, client resolution, commit, branch, dirty state, harness origin, scenario, comparison identity, and execution order so mismatched runs remain visible rather than silently comparable.

## Verify The Aggregator

```powershell
python -m unittest discover -s .\Benchmarks\tests -v
```

The tests use only the Python standard library and cover grouping boundaries, arithmetic statistics, the Student's t interval, legacy summaries, and aggregate-only HTML output.

## Regenerate The Report Only

```powershell
python .\Benchmarks\generate_report.py `
  --results .\Benchmarks\results `
  --output .\Benchmarks\results\report.html
```

The generator uses only Python's standard library and embeds all styles and charts into the HTML file.
