#!/usr/bin/env python3
"""Generate a self-contained HTML comparison report from AGP benchmark runs."""

from __future__ import annotations

import argparse
import csv
import html
import json
import math
import statistics
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class Run:
    summary_path: Path
    data: dict[str, Any]
    frame_times: tuple[float, ...]

    @property
    def key(self) -> tuple[str, str, str, str, str, str, str]:
        system = self.data.get("system", {})
        return (
            str(self.data.get("label", "unlabelled")),
            str(self.data.get("commit", "unknown")),
            str(self.data.get("configuration", "unknown")),
            str(system.get("adapter", "unknown")),
            str(system.get("computer", "unknown")),
            str(system.get("width", "unknown")),
            str(system.get("height", "unknown")),
        )


@dataclass(frozen=True)
class Group:
    label: str
    commit: str
    branch: str
    configuration: str
    adapter: str
    resolution: str
    runs: tuple[Run, ...]
    fps: float
    one_percent_low: float
    frame_mean: float
    frame_p95: float
    frame_p99: float
    spread: float


def median(values: list[float]) -> float:
    return statistics.median(values) if values else 0.0


def metric(run: Run, *path: str) -> float:
    value: Any = run.data
    for part in path:
        value = value[part]
    return float(value)


def load_runs(results_root: Path) -> list[Run]:
    runs: list[Run] = []
    for summary_path in sorted(results_root.glob("**/summary.json")):
        try:
            data = json.loads(summary_path.read_text(encoding="utf-8"))
            frames_path = summary_path.with_name("frames.csv")
            frame_times: list[float] = []
            if frames_path.exists():
                with frames_path.open(newline="", encoding="utf-8") as stream:
                    for row in csv.DictReader(stream):
                        frame_times.append(float(row["frame_ms"]))
            runs.append(Run(summary_path, data, tuple(frame_times)))
        except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
            print(f"warning: skipped {summary_path}: {error}")
    return runs


def build_groups(runs: list[Run]) -> list[Group]:
    grouped: dict[tuple[str, str, str, str, str, str, str], list[Run]] = defaultdict(list)
    for run in runs:
        grouped[run.key].append(run)

    groups: list[Group] = []
    for key, members in grouped.items():
        label, commit, configuration, adapter, _, _, _ = key
        first = members[0].data
        system = first.get("system", {})
        fps_values = [metric(run, "metrics", "average_fps") for run in members]
        groups.append(
            Group(
                label=label,
                commit=commit,
                branch=str(first.get("branch", "unknown")),
                configuration=configuration,
                adapter=adapter,
                resolution=f"{system.get('width', '?')}x{system.get('height', '?')}",
                runs=tuple(members),
                fps=median(fps_values),
                one_percent_low=median(
                    [metric(run, "metrics", "one_percent_low_fps") for run in members]
                ),
                frame_mean=median(
                    [metric(run, "metrics", "frame_ms", "mean") for run in members]
                ),
                frame_p95=median(
                    [metric(run, "metrics", "frame_ms", "p95") for run in members]
                ),
                frame_p99=median(
                    [metric(run, "metrics", "frame_ms", "p99") for run in members]
                ),
                spread=(max(fps_values) - min(fps_values)) if len(fps_values) > 1 else 0.0,
            )
        )
    return sorted(groups, key=lambda group: min(str(run.data.get("timestamp_utc", "")) for run in group.runs))


def bar_chart(groups: list[Group], title: str, subtitle: str, getter: Any, higher: bool) -> str:
    values = [getter(group) for group in groups]
    maximum = max(values, default=1.0) or 1.0
    rows: list[str] = []
    for index, (group, value) in enumerate(zip(groups, values)):
        width = max(2.0, 100.0 * value / maximum)
        color_class = f"c{index % 6}"
        direction = "higher is better" if higher else "lower is better"
        rows.append(
            f'<div class="bar-row" title="{html.escape(direction)}">'
            f'<div class="bar-label"><strong>{html.escape(group.label)}</strong>'
            f'<span>{html.escape(group.commit[:8])}</span></div>'
            f'<div class="bar-track"><div class="bar {color_class}" style="width:{width:.2f}%"></div></div>'
            f'<div class="bar-value">{value:.2f}</div></div>'
        )
    return (
        f'<section class="panel"><div class="panel-heading"><div><h2>{html.escape(title)}</h2>'
        f'<p>{html.escape(subtitle)}</p></div></div>{"".join(rows)}</section>'
    )


def sparkline(values: tuple[float, ...], color_index: int) -> str:
    if not values:
        return '<span class="muted">raw samples unavailable</span>'
    sampled = values[:: max(1, len(values) // 180)]
    cap = sorted(sampled)[max(0, math.floor(len(sampled) * 0.99) - 1)] or 1.0
    points: list[str] = []
    for index, value in enumerate(sampled):
        x = 100.0 * index / max(1, len(sampled) - 1)
        y = 31.0 - min(30.0, 30.0 * value / cap)
        points.append(f"{x:.2f},{y:.2f}")
    return (
        '<svg class="spark" viewBox="0 0 100 32" preserveAspectRatio="none" aria-label="Frame-time history">'
        f'<polyline class="spark-line c{color_index % 6}-stroke" points="{" ".join(points)}"/></svg>'
    )


def comparison_table(groups: list[Group]) -> str:
    if not groups:
        return ""
    baseline = groups[0]
    rows: list[str] = []
    for index, group in enumerate(groups):
        fps_delta = ((group.fps / baseline.fps) - 1.0) * 100.0 if baseline.fps else 0.0
        frame_delta = ((group.frame_mean / baseline.frame_mean) - 1.0) * 100.0 if baseline.frame_mean else 0.0
        dirty = any(bool(run.data.get("source_dirty")) for run in group.runs)
        dirty_badge = '<span class="warning">dirty source</span>' if dirty else ""
        rows.append(
            "<tr>"
            f'<td><span class="dot c{index % 6}"></span><strong>{html.escape(group.label)}</strong><br>'
            f'<span class="muted">{html.escape(group.branch)} - {html.escape(group.commit[:8])}</span></td>'
            f'<td>{group.fps:.2f}<small>{fps_delta:+.1f}%</small></td>'
            f'<td>{group.one_percent_low:.2f}</td>'
            f'<td>{group.frame_mean:.3f}<small>{frame_delta:+.1f}%</small></td>'
            f'<td>{group.frame_p95:.3f}</td><td>{group.frame_p99:.3f}</td>'
            f'<td>{len(group.runs)}<small>spread {group.spread:.2f} fps</small></td>'
            f'<td>{html.escape(group.configuration)}<small>{html.escape(group.resolution)}</small>'
            f'{dirty_badge}</td>'
            "</tr>"
        )
    return (
        '<section class="panel table-panel"><div class="panel-heading"><div><h2>Comparison</h2>'
        f'<p>Deltas use <strong>{html.escape(baseline.label)}</strong> as the first-run baseline.</p></div></div>'
        '<div class="table-wrap"><table><thead><tr><th>Version</th><th>Avg FPS</th><th>1% low</th>'
        '<th>Avg ms</th><th>P95 ms</th><th>P99 ms</th><th>Runs</th><th>Setup</th></tr></thead>'
        f'<tbody>{"".join(rows)}</tbody></table></div></section>'
    )


def render_report(groups: list[Group], runs: list[Run]) -> str:
    systems = {
        (group.adapter, group.resolution, group.configuration) for group in groups
    }
    warnings: list[str] = []
    if len(systems) > 1:
        warnings.append("Runs use mixed adapters, resolutions, or build configurations; compare them cautiously.")
    if any(bool(run.data.get("source_dirty")) for run in runs):
        warnings.append("At least one run used a dirty source tree. Keep only intentional benchmark harness changes.")

    warning_html = "".join(f'<div class="notice">{html.escape(message)}</div>' for message in warnings)
    run_cards: list[str] = []
    for index, run in enumerate(runs):
        data = run.data
        metrics = data["metrics"]
        run_cards.append(
            '<article class="run-card">'
            f'<div><strong>{html.escape(str(data.get("label", "unlabelled")))}</strong>'
            f'<span>{html.escape(str(data.get("commit", "unknown"))[:8])} · run {html.escape(str(data.get("run_index", "?")))}</span></div>'
            f'<b>{float(metrics["average_fps"]):.2f}<small> fps</small></b>'
            f'{sparkline(run.frame_times, index)}</article>'
        )

    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>AGP Engine Benchmark</title>
<style>
:root{{--ink:#15201d;--muted:#63706c;--paper:#f4f1e9;--panel:#fffdf8;--line:#d9d6cc;--accent:#1f6f5f}}
*{{box-sizing:border-box}}body{{margin:0;background:var(--paper);color:var(--ink);font:15px/1.45 Inter,Segoe UI,sans-serif}}
main{{max-width:1180px;margin:auto;padding:56px 28px 80px}}header{{display:flex;justify-content:space-between;gap:32px;align-items:end;margin-bottom:32px}}
.eyebrow{{font-size:12px;letter-spacing:.16em;text-transform:uppercase;color:var(--accent);font-weight:700}}h1{{font:600 46px/1.05 Georgia,serif;margin:8px 0 10px}}h2{{margin:0;font:600 22px/1.2 Georgia,serif}}p{{margin:0;color:var(--muted)}}
.stamp{{text-align:right;color:var(--muted)}}.grid{{display:grid;grid-template-columns:1fr 1fr;gap:18px}}.panel{{background:var(--panel);border:1px solid var(--line);border-radius:16px;padding:24px;box-shadow:0 8px 30px #23332c0a;margin-bottom:18px}}
.panel-heading{{display:flex;justify-content:space-between;margin-bottom:22px}}.bar-row{{display:grid;grid-template-columns:145px 1fr 64px;gap:14px;align-items:center;margin:14px 0}}.bar-label{{display:flex;flex-direction:column;overflow:hidden}}.bar-label strong{{white-space:nowrap;text-overflow:ellipsis;overflow:hidden}}.bar-label span,.muted{{color:var(--muted);font-size:12px}}.bar-track{{height:12px;background:#ece9e1;border-radius:9px;overflow:hidden}}.bar{{height:100%;border-radius:9px}}.bar-value{{font-variant-numeric:tabular-nums;text-align:right;font-weight:650}}
.c0{{background:#1f6f5f}}.c1{{background:#cf6a45}}.c2{{background:#4776a8}}.c3{{background:#9b7a32}}.c4{{background:#7a5b9d}}.c5{{background:#3f817c}}.c0-stroke{{stroke:#1f6f5f}}.c1-stroke{{stroke:#cf6a45}}.c2-stroke{{stroke:#4776a8}}.c3-stroke{{stroke:#9b7a32}}.c4-stroke{{stroke:#7a5b9d}}.c5-stroke{{stroke:#3f817c}}
.notice{{padding:12px 16px;border:1px solid #dabd76;background:#fff7df;border-radius:10px;margin-bottom:12px}}.table-wrap{{overflow:auto}}table{{width:100%;border-collapse:collapse}}th{{color:var(--muted);font-size:11px;text-transform:uppercase;letter-spacing:.08em;text-align:left}}th,td{{padding:13px 12px;border-bottom:1px solid var(--line);white-space:nowrap}}td{{font-variant-numeric:tabular-nums}}td small{{display:block;color:var(--muted)}}.dot{{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:8px}}.warning{{display:block;color:#a14c2e;font-size:11px;font-weight:700}}
.run-grid{{display:grid;grid-template-columns:repeat(2,1fr);gap:12px}}.run-card{{display:grid;grid-template-columns:150px 85px 1fr;gap:14px;align-items:center;padding:15px;border:1px solid var(--line);border-radius:12px}}.run-card div{{display:flex;flex-direction:column}}.run-card b{{font-size:19px;font-variant-numeric:tabular-nums}}.run-card small{{font-weight:400;color:var(--muted)}}.spark{{width:100%;height:42px}}.spark-line{{fill:none;stroke-width:1.3;vector-effect:non-scaling-stroke}}
.method{{display:grid;grid-template-columns:repeat(3,1fr);gap:18px}}.method div{{border-left:2px solid var(--line);padding-left:14px}}.method strong{{display:block;margin-bottom:4px}}footer{{color:var(--muted);font-size:12px;margin-top:24px}}
@media(max-width:800px){{header{{display:block}}.stamp{{text-align:left;margin-top:16px}}.grid,.run-grid,.method{{grid-template-columns:1fr}}.bar-row{{grid-template-columns:110px 1fr 55px}}.run-card{{grid-template-columns:120px 72px 1fr}}h1{{font-size:36px}}}}
</style></head><body><main>
<header><div><div class="eyebrow">AGP · repeatable performance evidence</div><h1>Engine benchmark</h1><p>Steady-state ModelViewer throughput, compared commit by commit.</p></div><div class="stamp">{len(runs)} runs<br>{len(groups)} compared versions</div></header>
{warning_html}
<div class="grid">
{bar_chart(groups, "Average throughput", "Frames per second · higher is better", lambda group: group.fps, True)}
{bar_chart(groups, "Slow-frame cost", "P95 frame time in milliseconds · lower is better", lambda group: group.frame_p95, False)}
</div>
{comparison_table(groups)}
<section class="panel"><div class="panel-heading"><div><h2>Run consistency</h2><p>Raw frame-time traces make spikes and unstable runs visible.</p></div></div><div class="run-grid">{"".join(run_cards)}</div></section>
<section class="panel"><div class="panel-heading"><div><h2>How to read this</h2><p>The benchmark intentionally keeps the first version as the reference point.</p></div></div><div class="method"><div><strong>Average FPS</strong><p>Overall throughput across measured frames after warmup.</p></div><div><strong>1% low</strong><p>FPS derived from the slowest one percent of frames. Higher means fewer noticeable hitches.</p></div><div><strong>P95 frame time</strong><p>Ninety-five percent of frames completed at or below this cost. Lower is better.</p></div></div></section>
<footer>CPU-observed frame intervals measured between completed Present calls. This captures application-side throughput and driver back-pressure; it is not a GPU timestamp profile.</footer>
</main></body></html>"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    runs = load_runs(args.results)
    if not runs:
        raise SystemExit(f"No summary.json files found under {args.results}")
    groups = build_groups(runs)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_report(groups, runs), encoding="utf-8")
    print(f"Generated {args.output} from {len(runs)} run(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
