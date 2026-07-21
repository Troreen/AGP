# AGP Engine Benchmarks

This benchmark answers one deliberately simple question:

> With the same ModelViewer scene, machine, resolution, and Release build, how much steady-state frame throughput changed between engine commits?

The harness measures CPU-observed frame intervals at the shared RHI `Present` boundary. That location works for both the old main-branch application loop and the optimized snapshot/threaded loop, so an optimization does not need benchmark-specific integration.

## What Each Run Saves

Every repetition creates a timestamped directory under `Benchmarks/results` containing:

- `summary.json`: commit, branch, build, hardware, resolution, run settings, average FPS, 1% low FPS, and frame/present percentiles.
- `frames.csv`: every measured frame interval and `Present` duration for spike inspection or external analysis.

`Benchmarks/results/report.html` is regenerated from all summaries. It contains simple comparison bars, a baseline-relative table, and raw frame-time traces. It is self-contained and can be opened directly in any normal browser.

Results are intentionally not ignored by Git. Commit the raw results and regenerated report after a real comparison so the evidence stays beside the optimization history. Do not commit short smoke-test runs.

## Normal Run

Use a clean, committed checkout and close unrelated heavy programs:

```powershell
.\Benchmarks\run.ps1 `
  -Label "before-thread-pool" `
  -WarmupFrames 300 `
  -SampleFrames 1200 `
  -Repetitions 3 `
  -Notes "ModelViewer default scene"
```

The script:

1. Builds `ModelViewer` as `Release | x64`.
2. Links the freshly built shaders into the expected ignored runtime location and provides a temporary `Assets` junction only for historical refs with legacy path assumptions.
3. Discards the warmup frames.
4. Saves the requested number of raw frame samples.
5. Closes ModelViewer automatically.
6. Repeats the run and regenerates `report.html`.

Three repetitions are the default because a single desktop run can be noisy. The report groups repetitions from the same label/commit and uses their median result.

## Compare Existing Commits

Keep benchmark harness changes separate from engine optimizations. Pass the latest harness setup commit to `compare.ps1`; it traces back to the commit that introduced `FrameBenchmark.cpp` and extracts the complete instrumentation range. Older refs can then be measured with the exact same harness without rewriting this branch:

```powershell
.\Benchmarks\compare.ps1 `
  -Refs main,2f91e36,8c0efbb `
  -HarnessCommit <benchmark-harness-commit> `
  -WarmupFrames 300 `
  -SampleFrames 1200 `
  -Repetitions 3
```

For every ref, `compare.ps1` creates a detached temporary worktree, extracts and applies only the benchmark source/project instrumentation from the benchmark commit when necessary, builds and runs it, writes the results back to this checkout, and removes the temporary worktree. Documentation and result files are deliberately excluded from that temporary patch. The measured commit stored in the result remains the requested ref, while the `harness` field records how instrumentation was supplied.

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
5. Open `Benchmarks/results/report.html` and inspect average FPS, 1% low, P95, and the traces.
6. Commit the result directories and report as performance evidence.
7. Record the conclusion: improved, neutral within noise, or regressed.

If the result is smaller than the run-to-run spread, call it neutral and collect more repetitions. Do not claim an improvement based on one unusually good run.

## Reading The Metrics

- **Average FPS:** overall throughput. Higher is better.
- **Average frame time:** the same throughput expressed in milliseconds. Lower is better.
- **1% low FPS:** FPS derived from the slowest one percent of measured frames. Higher means fewer visible hitches.
- **P95/P99 frame time:** 95%/99% of frames completed at or below this cost. Lower is better.
- **Present time:** time spent inside DXGI `Present`. Use it as supporting diagnostic data, not as the optimization score.
- **Run spread:** difference between the fastest and slowest repetition. A proposed gain should be meaningfully larger than this noise.

The headline numbers are CPU-observed frame intervals, not Direct3D GPU timestamp queries. They include application work, command submission, and driver back-pressure. They are a good first benchmark for the current CPU/scheduling optimizations. Add GPU timestamps later if an optimization specifically targets shader or GPU cost.

## Fair-Comparison Rules

- Use `Release | x64`.
- Keep the ModelViewer window, default scene, camera, and light state unchanged.
- Use the same machine, power mode, GPU, resolution, warmup, and sample count.
- Run versions in one session when possible.
- Avoid interacting with the benchmark window.
- Reject dirty-source runs unless the only difference is the explicitly recorded temporary harness.
- Look at repetition spread and traces before trusting a percentage.

The JSON summary stores configuration, CPU identifier, GPU adapter, client resolution, commit, branch, dirty state, and harness origin so mismatched runs remain visible rather than silently comparable.

## Regenerate The Report Only

```powershell
python .\Benchmarks\generate_report.py `
  --results .\Benchmarks\results `
  --output .\Benchmarks\results\report.html
```

The generator uses only Python's standard library and embeds all styles and charts into the HTML file.
