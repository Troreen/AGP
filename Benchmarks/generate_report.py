#!/usr/bin/env python3
"""Generate self-contained aggregate reports from AGP benchmark summaries."""

from __future__ import annotations

import argparse
import html
import json
import math
import statistics
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


T_CRITICAL_95 = {
    1: 12.706,
    2: 4.303,
    3: 3.182,
    4: 2.776,
    5: 2.571,
    6: 2.447,
    7: 2.365,
    8: 2.306,
    9: 2.262,
    10: 2.228,
    11: 2.201,
    12: 2.179,
    13: 2.160,
    14: 2.145,
    15: 2.131,
    16: 2.120,
    17: 2.110,
    18: 2.101,
    19: 2.093,
    20: 2.086,
    21: 2.080,
    22: 2.074,
    23: 2.069,
    24: 2.064,
    25: 2.060,
    26: 2.056,
    27: 2.052,
    28: 2.048,
    29: 2.045,
    30: 2.042,
}


@dataclass(frozen=True)
class MetricSpec:
    key: str
    label: str
    path: tuple[str, ...]
    higher_is_better: bool
    decimals: int


METRICS = (
    MetricSpec("average_fps", "Average FPS", ("metrics", "average_fps"), True, 2),
    MetricSpec(
        "one_percent_low_fps",
        "1% low FPS",
        ("metrics", "one_percent_low_fps"),
        True,
        2,
    ),
    MetricSpec(
        "frame_mean",
        "Mean frame time (ms)",
        ("metrics", "frame_ms", "mean"),
        False,
        3,
    ),
    MetricSpec(
        "frame_p95",
        "P95 frame time (ms)",
        ("metrics", "frame_ms", "p95"),
        False,
        3,
    ),
    MetricSpec(
        "frame_p99",
        "P99 frame time (ms)",
        ("metrics", "frame_ms", "p99"),
        False,
        3,
    ),
    MetricSpec(
        "present_mean",
        "Mean Present time (ms)",
        ("metrics", "present_ms", "mean"),
        False,
        3,
    ),
)
METRIC_BY_KEY = {spec.key: spec for spec in METRICS}


@dataclass(frozen=True)
class Run:
    summary_path: Path
    data: dict[str, Any]

    @property
    def comparison_id(self) -> str:
        value = str(self.data.get("comparison_id", "")).strip()
        return value if value else "legacy"

    @property
    def scenario(self) -> str:
        return str(self.data.get("scenario", "default"))

    @property
    def is_legacy(self) -> bool:
        return self.comparison_id == "legacy"

    @property
    def key(self) -> tuple[str, str, str, str, str, str, str, str, str]:
        system = self.data.get("system", {})
        return (
            self.comparison_id,
            self.scenario,
            str(self.data.get("label", "unlabelled")),
            str(self.data.get("commit", "unknown")),
            str(self.data.get("configuration", "unknown")),
            str(system.get("computer", "unknown")),
            str(system.get("adapter", "unknown")),
            str(system.get("width", "unknown")),
            str(system.get("height", "unknown")),
        )


@dataclass(frozen=True)
class Aggregate:
    count: int
    mean: float
    median: float
    sample_stddev: float | None
    minimum: float
    maximum: float
    coefficient_of_variation: float | None
    ci95_low: float | None
    ci95_high: float | None


@dataclass(frozen=True)
class Group:
    comparison_id: str
    scenario: str
    label: str
    commit: str
    branch: str
    requested_ref: str
    configuration: str
    computer: str
    adapter: str
    resolution: str
    runs: tuple[Run, ...]
    aggregates: dict[str, Aggregate]


def metric(run: Run, path: tuple[str, ...]) -> float:
    value: Any = run.data
    for part in path:
        value = value[part]
    return float(value)


def t_critical_95(sample_count: int) -> float:
    degrees_of_freedom = sample_count - 1
    if degrees_of_freedom < 1:
        raise ValueError("A confidence interval requires at least two samples.")
    return T_CRITICAL_95.get(degrees_of_freedom, 1.96)


def calculate_aggregate(values: Iterable[float]) -> Aggregate:
    samples = [float(value) for value in values]
    if not samples:
        raise ValueError("Cannot aggregate an empty sample.")

    mean = statistics.fmean(samples)
    median = statistics.median(samples)
    sample_stddev: float | None = None
    coefficient_of_variation: float | None = None
    ci95_low: float | None = None
    ci95_high: float | None = None
    if len(samples) >= 2:
        sample_stddev = statistics.stdev(samples)
        if mean != 0.0:
            coefficient_of_variation = sample_stddev / abs(mean) * 100.0
        margin = t_critical_95(len(samples)) * sample_stddev / math.sqrt(len(samples))
        ci95_low = mean - margin
        ci95_high = mean + margin

    return Aggregate(
        count=len(samples),
        mean=mean,
        median=median,
        sample_stddev=sample_stddev,
        minimum=min(samples),
        maximum=max(samples),
        coefficient_of_variation=coefficient_of_variation,
        ci95_low=ci95_low,
        ci95_high=ci95_high,
    )


def load_runs(results_root: Path) -> list[Run]:
    runs: list[Run] = []
    for summary_path in sorted(results_root.glob("**/summary.json")):
        try:
            data = json.loads(summary_path.read_text(encoding="utf-8"))
            runs.append(Run(summary_path, data))
        except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
            print(f"warning: skipped {summary_path}: {error}")
    return runs


def _run_order(run: Run) -> tuple[int, int, str]:
    try:
        execution_index = int(run.data.get("execution_index", 0))
    except (TypeError, ValueError):
        execution_index = 0
    try:
        run_index = int(run.data.get("run_index", 0))
    except (TypeError, ValueError):
        run_index = 0
    return execution_index, run_index, str(run.data.get("timestamp_utc", ""))


def build_groups(runs: list[Run]) -> list[Group]:
    grouped: dict[
        tuple[str, str, str, str, str, str, str, str, str], list[Run]
    ] = defaultdict(list)
    for run in runs:
        grouped[run.key].append(run)

    groups: list[Group] = []
    for key, members in grouped.items():
        (
            comparison_id,
            scenario,
            label,
            commit,
            configuration,
            computer,
            adapter,
            width,
            height,
        ) = key
        ordered_members = tuple(sorted(members, key=_run_order))
        first = ordered_members[0].data
        aggregates = {
            spec.key: calculate_aggregate(
                metric(run, spec.path) for run in ordered_members
            )
            for spec in METRICS
        }
        groups.append(
            Group(
                comparison_id=comparison_id,
                scenario=scenario,
                label=label,
                commit=commit,
                branch=str(first.get("branch", "unknown")),
                requested_ref=str(first.get("requested_ref", label)),
                configuration=configuration,
                computer=computer,
                adapter=adapter,
                resolution=f"{width}x{height}",
                runs=ordered_members,
                aggregates=aggregates,
            )
        )

    return sorted(
        groups,
        key=lambda group: (
            min(str(run.data.get("timestamp_utc", "")) for run in group.runs),
            group.comparison_id,
            group.scenario,
            group.label,
        ),
    )


def group_sessions(groups: list[Group]) -> list[tuple[tuple[str, str], list[Group]]]:
    sessions: dict[tuple[str, str], list[Group]] = defaultdict(list)
    session_order: dict[tuple[str, str], str] = {}
    for group in groups:
        key = (group.comparison_id, group.scenario)
        sessions[key].append(group)
        first_timestamp = min(
            str(run.data.get("timestamp_utc", "")) for run in group.runs
        )
        session_order[key] = min(session_order.get(key, first_timestamp), first_timestamp)
    return sorted(sessions.items(), key=lambda item: (session_order[item[0]], item[0]))


def _format(value: float, decimals: int) -> str:
    return f"{value:.{decimals}f}"


def _uncertainty_text(aggregate: Aggregate, decimals: int) -> str:
    if aggregate.ci95_low is None or aggregate.ci95_high is None:
        return "95% CI unavailable"
    return (
        f"95% CI {_format(aggregate.ci95_low, decimals)}–"
        f"{_format(aggregate.ci95_high, decimals)}"
    )


def _variation_text(aggregate: Aggregate, decimals: int) -> str:
    if (
        aggregate.sample_stddev is None
        or aggregate.coefficient_of_variation is None
    ):
        return "SD and CV unavailable"
    return (
        f"SD {_format(aggregate.sample_stddev, decimals)} · "
        f"CV {aggregate.coefficient_of_variation:.2f}%"
    )


def aggregate_chart(groups: list[Group], spec: MetricSpec) -> str:
    aggregates = [group.aggregates[spec.key] for group in groups]
    bounds = [
        value
        for aggregate in aggregates
        for value in (
            aggregate.ci95_low if aggregate.ci95_low is not None else aggregate.mean,
            aggregate.ci95_high if aggregate.ci95_high is not None else aggregate.mean,
        )
    ]
    scale_min = min(bounds, default=0.0)
    scale_max = max(bounds, default=1.0)
    span = scale_max - scale_min
    if span <= 0.0:
        span = max(abs(scale_max) * 0.1, 1.0)
    padding = span * 0.12
    scale_min -= padding
    scale_max += padding
    scale_span = scale_max - scale_min

    rows: list[str] = []
    for index, (group, aggregate) in enumerate(zip(groups, aggregates)):
        mean_position = (aggregate.mean - scale_min) / scale_span * 100.0
        ci_low = (
            aggregate.ci95_low
            if aggregate.ci95_low is not None
            else aggregate.mean
        )
        ci_high = (
            aggregate.ci95_high
            if aggregate.ci95_high is not None
            else aggregate.mean
        )
        whisker_left = (ci_low - scale_min) / scale_span * 100.0
        whisker_width = max(0.0, (ci_high - ci_low) / scale_span * 100.0)
        variation_text = _variation_text(aggregate, spec.decimals)
        rows.append(
            '<div class="aggregate-row">'
            f'<div class="version"><span class="dot c{index % 6}"></span>'
            f"<strong>{html.escape(group.label)}</strong>"
            f"<small>{html.escape(group.commit[:8])}</small></div>"
            '<div class="interval-track" aria-label="Mean and 95% confidence interval">'
            f'<span class="ci-whisker c{index % 6}" style="left:{whisker_left:.3f}%;'
            f'width:{whisker_width:.3f}%"></span>'
            f'<span class="mean-dot c{index % 6}" style="left:{mean_position:.3f}%"></span>'
            "</div>"
            '<div class="aggregate-value">'
            f"<strong>{_format(aggregate.mean, spec.decimals)}</strong>"
            f"<small>{html.escape(_uncertainty_text(aggregate, spec.decimals))}</small>"
            f"<small>median {_format(aggregate.median, spec.decimals)} · "
            f"{html.escape(variation_text)} · n={aggregate.count}</small></div></div>"
        )

    direction = "higher is better" if spec.higher_is_better else "lower is better"
    return (
        '<section class="metric-card">'
        f"<h3>{html.escape(spec.label)}</h3><p>Mean with Student's t 95% CI · {direction}</p>"
        f'<div class="axis"><span>{_format(scale_min, spec.decimals)}</span>'
        f"<span>{_format(scale_max, spec.decimals)}</span></div>"
        f'{"".join(rows)}</section>'
    )


def comparison_table(groups: list[Group]) -> str:
    headings = "".join(f"<th>{html.escape(spec.label)}</th>" for spec in METRICS)
    rows: list[str] = []
    for index, group in enumerate(groups):
        cells: list[str] = []
        for spec in METRICS:
            aggregate = group.aggregates[spec.key]
            variation_text = _variation_text(aggregate, spec.decimals)
            cells.append(
                f"<td><strong>{_format(aggregate.mean, spec.decimals)}</strong>"
                f"<small>{html.escape(_uncertainty_text(aggregate, spec.decimals))}</small>"
                f"<small>median {_format(aggregate.median, spec.decimals)} · "
                f"{html.escape(variation_text)}</small></td>"
            )
        dirty = any(bool(run.data.get("source_dirty")) for run in group.runs)
        dirty_badge = '<span class="warning">dirty source</span>' if dirty else ""
        rows.append(
            "<tr><td>"
            f'<span class="dot c{index % 6}"></span><strong>{html.escape(group.label)}</strong>'
            f"<small>{html.escape(group.branch)} · {html.escape(group.commit[:8])}</small>"
            f"<small>{html.escape(group.configuration)} · {html.escape(group.resolution)} · "
            f"n={len(group.runs)}</small>{dirty_badge}</td>"
            f'{"".join(cells)}</tr>'
        )
    return (
        '<div class="table-wrap"><table><thead><tr><th>Version</th>'
        f"{headings}</tr></thead><tbody>{''.join(rows)}</tbody></table></div>"
    )


def session_section(session_key: tuple[str, str], groups: list[Group]) -> str:
    comparison_id, scenario = session_key
    legacy = comparison_id == "legacy"
    title = (
        "Legacy summaries (no comparison_id)"
        if legacy
        else f"Comparison {comparison_id}"
    )
    systems = {
        (group.computer, group.adapter, group.resolution, group.configuration)
        for group in groups
    }
    notices: list[str] = []
    if legacy:
        notices.append(
            "These summaries predate comparison metadata. They are readable but never "
            "merged with identified comparison sessions."
        )
    if len(systems) > 1:
        notices.append(
            "This section contains multiple machines, adapters, resolutions, or "
            "configurations. They remain separate aggregates."
        )
    if any(bool(run.data.get("source_dirty")) for group in groups for run in group.runs):
        notices.append("At least one aggregate used a dirty source tree.")
    notice_html = "".join(
        f'<div class="notice">{html.escape(message)}</div>' for message in notices
    )
    featured = ("average_fps", "one_percent_low_fps", "frame_p95")
    charts = "".join(
        aggregate_chart(groups, METRIC_BY_KEY[key]) for key in featured
    )
    run_count = sum(len(group.runs) for group in groups)
    return (
        '<section class="session">'
        '<div class="session-heading"><div>'
        f'<div class="eyebrow">{html.escape(scenario)} scenario</div>'
        f"<h2>{html.escape(title)}</h2></div>"
        f"<div class=\"session-count\">{run_count} raw summaries<br>"
        f"{len(groups)} aggregates</div></div>"
        f"{notice_html}<div class=\"metric-grid\">{charts}</div>"
        '<section class="panel"><div class="panel-heading"><div>'
        "<h3>Aggregate details</h3>"
        "<p>Arithmetic mean is primary; median and run-to-run variation are secondary checks.</p>"
        f"</div></div>{comparison_table(groups)}</section></section>"
    )


def render_report(groups: list[Group], runs: list[Run]) -> str:
    sessions = group_sessions(groups)
    session_html = "".join(session_section(key, members) for key, members in sessions)
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>AGP Engine Benchmark</title>
<style>
:root{{--ink:#15201d;--muted:#63706c;--paper:#f4f1e9;--panel:#fffdf8;--line:#d9d6cc;--accent:#1f6f5f}}
*{{box-sizing:border-box}}body{{margin:0;background:var(--paper);color:var(--ink);font:15px/1.45 Inter,Segoe UI,sans-serif}}
main{{max-width:1280px;margin:auto;padding:56px 28px 80px}}header,.session-heading{{display:flex;justify-content:space-between;gap:32px;align-items:end;margin-bottom:32px}}
.eyebrow{{font-size:12px;letter-spacing:.16em;text-transform:uppercase;color:var(--accent);font-weight:700}}h1{{font:600 46px/1.05 Georgia,serif;margin:8px 0 10px}}h2{{font:600 30px/1.1 Georgia,serif;margin:6px 0}}h3{{margin:0;font:600 20px/1.2 Georgia,serif}}p{{margin:5px 0 0;color:var(--muted)}}
.stamp,.session-count{{text-align:right;color:var(--muted)}}.session{{border-top:1px solid var(--line);padding-top:38px;margin-top:38px}}.panel,.metric-card{{background:var(--panel);border:1px solid var(--line);border-radius:16px;padding:24px;box-shadow:0 8px 30px #23332c0a;margin-bottom:18px}}
.notice{{padding:12px 16px;border:1px solid #dabd76;background:#fff7df;border-radius:10px;margin-bottom:12px}}.metric-grid{{display:grid;grid-template-columns:repeat(3,1fr);gap:18px}}.metric-card h3{{margin-bottom:2px}}.metric-card>p{{font-size:12px;margin-bottom:20px}}
.axis{{display:flex;justify-content:space-between;color:var(--muted);font-size:11px;margin:0 118px 4px 132px}}.aggregate-row{{display:grid;grid-template-columns:120px minmax(120px,1fr) 190px;gap:12px;align-items:center;margin:16px 0}}.version,.aggregate-value{{display:flex;flex-direction:column;min-width:0}}.version strong{{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}}small{{display:block;color:var(--muted);font-size:11px}}.interval-track{{height:22px;position:relative;background:linear-gradient(to right,transparent 49.7%,#e4e1d9 50%,transparent 50.3%);border-bottom:1px solid var(--line)}}.ci-whisker{{position:absolute;top:9px;height:4px;border-radius:4px}}.ci-whisker:before,.ci-whisker:after{{content:\"\";position:absolute;top:-4px;width:1px;height:12px;background:inherit}}.ci-whisker:before{{left:0}}.ci-whisker:after{{right:0}}.mean-dot{{position:absolute;top:5px;width:12px;height:12px;margin-left:-6px;border:2px solid var(--panel);border-radius:50%;box-shadow:0 0 0 1px #ffffff80}}.aggregate-value strong{{font-size:17px;font-variant-numeric:tabular-nums}}
.c0{{background:#1f6f5f}}.c1{{background:#cf6a45}}.c2{{background:#4776a8}}.c3{{background:#9b7a32}}.c4{{background:#7a5b9d}}.c5{{background:#3f817c}}.dot{{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:8px}}
.panel-heading{{display:flex;justify-content:space-between;margin-bottom:22px}}.table-wrap{{overflow:auto}}table{{width:100%;border-collapse:collapse}}th{{color:var(--muted);font-size:10px;text-transform:uppercase;letter-spacing:.07em;text-align:left}}th,td{{padding:13px 12px;border-bottom:1px solid var(--line);white-space:nowrap;vertical-align:top}}td{{font-variant-numeric:tabular-nums}}.warning{{display:block;color:#a14c2e;font-size:11px;font-weight:700}}
.method{{display:grid;grid-template-columns:repeat(3,1fr);gap:18px}}.method div{{border-left:2px solid var(--line);padding-left:14px}}.method strong{{display:block;margin-bottom:4px}}footer{{color:var(--muted);font-size:12px;margin-top:28px}}
@media(max-width:1080px){{.metric-grid{{grid-template-columns:1fr}}}}@media(max-width:760px){{main{{padding:36px 16px 60px}}header,.session-heading{{display:block}}.stamp,.session-count{{text-align:left;margin-top:14px}}h1{{font-size:36px}}.aggregate-row{{grid-template-columns:100px 1fr}}.aggregate-value{{grid-column:1/-1}}.axis{{margin-left:112px;margin-right:0}}.method{{grid-template-columns:1fr}}}}
</style></head><body><main>
<header><div><div class="eyebrow">AGP · repeatable performance evidence</div><h1>Engine benchmark</h1><p>Independent comparison sessions and scenarios with run-to-run uncertainty.</p></div><div class="stamp">{len(runs)} raw summaries<br>{len(groups)} aggregates<br>{len(sessions)} sessions/scenarios</div></header>
{session_html}
<section class="panel"><div class="panel-heading"><div><h3>How to read this</h3><p>Confidence intervals quantify uncertainty in the run-level arithmetic mean.</p></div></div><div class="method"><div><strong>Throughput</strong><p>Average FPS is the primary throughput measure. Higher is better.</p></div><div><strong>Tail behavior</strong><p>1% low FPS and P95/P99 frame time expose hitching and slow frames.</p></div><div><strong>Consistency</strong><p>Sample standard deviation and coefficient of variation describe run-to-run stability.</p></div></div></section>
<footer>CPU-observed frame intervals measured between completed Present calls. Present time is diagnostic evidence, not a GPU timestamp or whole-process CPU profile. Raw frames.csv and summary.json files remain the source evidence.</footer>
</main></body></html>"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--comparison-id",
        help="Generate a focused report for one comparison ID (or 'legacy').",
    )
    args = parser.parse_args()

    runs = load_runs(args.results)
    if args.comparison_id:
        runs = [run for run in runs if run.comparison_id == args.comparison_id]
    if not runs:
        suffix = (
            f" for comparison {args.comparison_id}"
            if args.comparison_id
            else ""
        )
        raise SystemExit(f"No summary.json files found under {args.results}{suffix}")
    groups = build_groups(runs)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_report(groups, runs), encoding="utf-8")
    print(
        f"Generated {args.output} from {len(runs)} run(s) in "
        f"{len(group_sessions(groups))} session/scenario section(s)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
