from __future__ import annotations

import importlib.util
import math
import statistics
import sys
import unittest
from pathlib import Path


REPORT_PATH = Path(__file__).parents[1] / "generate_report.py"
SPEC = importlib.util.spec_from_file_location("generate_report", REPORT_PATH)
assert SPEC is not None and SPEC.loader is not None
generate_report = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = generate_report
SPEC.loader.exec_module(generate_report)


def make_run(
    index: int,
    value: float,
    *,
    comparison_id: str | None = "comparison-a",
    scenario: str = "default",
    label: str = "main",
    commit: str = "a" * 40,
    configuration: str = "Release",
    computer: str = "test-machine",
    adapter: str = "test-adapter",
    width: int = 1280,
    height: int = 720,
) -> generate_report.Run:
    data = {
        "schema_version": 2 if comparison_id else 1,
        "timestamp_utc": f"2026-07-21T00:00:{index:02d}Z",
        "label": label,
        "commit": commit,
        "branch": label,
        "configuration": configuration,
        "run_index": index,
        "execution_index": index,
        "requested_ref": label,
        "scenario": scenario,
        "system": {
            "computer": computer,
            "adapter": adapter,
            "width": width,
            "height": height,
        },
        "metrics": {
            "average_fps": value,
            "one_percent_low_fps": value - 1.0,
            "frame_ms": {
                "mean": value + 1.0,
                "p95": value + 2.0,
                "p99": value + 3.0,
            },
            "present_ms": {"mean": value + 4.0},
        },
    }
    if comparison_id is not None:
        data["comparison_id"] = comparison_id
    return generate_report.Run(Path(f"run-{index}") / "summary.json", data)


class AggregateTests(unittest.TestCase):
    def test_ten_runs_form_one_aggregate(self) -> None:
        groups = generate_report.build_groups(
            [make_run(index, float(index)) for index in range(1, 11)]
        )

        self.assertEqual(len(groups), 1)
        aggregate = groups[0].aggregates["average_fps"]
        self.assertEqual(aggregate.count, 10)
        self.assertEqual(len(groups[0].runs), 10)

    def test_dimensions_never_merge(self) -> None:
        runs = [
            make_run(1, 1.0),
            make_run(2, 1.0, comparison_id="comparison-b"),
            make_run(3, 1.0, scenario="busy"),
            make_run(4, 1.0, commit="b" * 40),
            make_run(5, 1.0, configuration="Debug"),
            make_run(6, 1.0, computer="other-machine"),
            make_run(7, 1.0, adapter="other-adapter"),
            make_run(8, 1.0, width=1920, height=1080),
        ]

        groups = generate_report.build_groups(runs)

        self.assertEqual(len(groups), len(runs))

    def test_statistics_and_student_t_interval_are_correct(self) -> None:
        values = [float(value) for value in range(1, 11)]
        aggregate = generate_report.calculate_aggregate(values)
        expected_stddev = statistics.stdev(values)
        expected_margin = 2.262 * expected_stddev / math.sqrt(10)

        self.assertAlmostEqual(aggregate.mean, 5.5)
        self.assertAlmostEqual(aggregate.median, 5.5)
        self.assertAlmostEqual(aggregate.sample_stddev or 0.0, expected_stddev)
        self.assertAlmostEqual(
            aggregate.coefficient_of_variation or 0.0,
            expected_stddev / 5.5 * 100.0,
        )
        self.assertAlmostEqual(aggregate.ci95_low or 0.0, 5.5 - expected_margin)
        self.assertAlmostEqual(aggregate.ci95_high or 0.0, 5.5 + expected_margin)
        self.assertEqual(aggregate.minimum, 1.0)
        self.assertEqual(aggregate.maximum, 10.0)

    def test_single_run_marks_variation_and_interval_unavailable(self) -> None:
        aggregate = generate_report.calculate_aggregate([4.0])

        self.assertEqual(aggregate.mean, 4.0)
        self.assertIsNone(aggregate.sample_stddev)
        self.assertIsNone(aggregate.coefficient_of_variation)
        self.assertIsNone(aggregate.ci95_low)
        self.assertIsNone(aggregate.ci95_high)

    def test_legacy_data_stays_separate_and_report_is_aggregate_only(self) -> None:
        runs = [
            make_run(1, 10.0, comparison_id=None),
            make_run(2, 11.0, comparison_id=None),
            make_run(1, 12.0, comparison_id="comparison-a"),
            make_run(2, 13.0, comparison_id="comparison-a"),
        ]
        groups = generate_report.build_groups(runs)
        report = generate_report.render_report(groups, runs)

        self.assertEqual(
            {group.comparison_id for group in groups},
            {"legacy", "comparison-a"},
        )
        self.assertIn("Legacy summaries (no comparison_id)", report)
        self.assertIn("ci-whisker", report)
        self.assertIn("95% CI", report)
        self.assertNotIn("run-card", report)
        self.assertNotIn("Frame-time history", report)


if __name__ == "__main__":
    unittest.main()
