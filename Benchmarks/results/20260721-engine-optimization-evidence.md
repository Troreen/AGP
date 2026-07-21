# Engine Optimization Evidence

## Method

- Machine: DESKTOP-O2EOU65, NVIDIA GeForce MX450, Release x64, 1284 x 701 client area.
- Compared refs: main at 7e1538c4 and engine-optimizations at 0c803e14.
- Harness: 0c803e14.
- Ten repetitions per ref and scenario, built once per ref and executed in balanced interleaved order.
- Every run used 300 warmup frames and 1,200 measured frames.
- Values below are arithmetic means across runs with two-sided Student's t 95% confidence intervals.

## Default scenario

Comparison ID: evidence-default-20260721-0c803e1

| Metric | main | engine-optimizations | Change | Interpretation |
|---|---:|---:|---:|---|
| Average FPS | 338.30 [314.89, 361.71] | 375.88 [369.23, 382.53] | +11.11% | Improved |
| 1% low FPS | 98.40 [76.77, 120.04] | 110.04 [95.41, 124.68] | +11.83% | Neutral; intervals overlap |
| Mean frame time | 2.987 ms [2.734, 3.239] | 2.662 ms [2.614, 2.710] | -10.87% | Improved |
| P95 frame time | 6.660 ms [6.237, 7.083] | 5.788 ms [5.465, 6.111] | -13.09% | Improved |
| P99 frame time | 8.903 ms [7.416, 10.391] | 7.250 ms [6.806, 7.694] | -18.57% | Favorable, but intervals overlap |
| Mean Present time | 2.090 ms [1.956, 2.223] | 1.696 ms [1.626, 1.766] | -18.86% | Diagnostic only |

Average-FPS coefficient of variation fell from 9.67% to 2.47%. The optimized ref had higher average FPS in all ten repetition pairs. This is an improvement in default-scene throughput and consistency, with P95 evidence supporting better tail behavior; the 1% low result remains neutral under uncertainty.

## Busy scenario

Comparison ID: evidence-busy-20260721-0c803e1

| Metric | main | engine-optimizations | Change | Interpretation |
|---|---:|---:|---:|---|
| Average FPS | 88.32 [71.97, 104.67] | 162.38 [119.24, 205.51] | +83.85% | Improved |
| 1% low FPS | 39.52 [27.73, 51.30] | 61.43 [36.43, 86.43] | +55.46% | Favorable, but intervals overlap |
| Mean frame time | 12.739 ms [8.164, 17.314] | 7.324 ms [4.696, 9.952] | -42.51% | Favorable, but intervals overlap |
| P95 frame time | 19.150 ms [12.510, 25.789] | 12.255 ms [6.575, 17.934] | -36.01% | Favorable, but intervals overlap |
| P99 frame time | 24.353 ms [16.481, 32.225] | 18.065 ms [9.992, 26.138] | -25.82% | Favorable, but intervals overlap |
| Mean Present time | 0.097 ms [0.054, 0.139] | 0.101 ms [0.065, 0.137] | +4.53% | Neutral; diagnostic only |

The optimized ref had higher average FPS in nine of ten repetition pairs, and the average-FPS confidence intervals do not overlap. That supports an improved busy-scene throughput result. However, average-FPS coefficients of variation were high for both refs (25.88% and 37.14%), and the tail-metric intervals overlap. Treat the tail improvements as directional evidence rather than a conclusive consistency win.

## Conclusion and limits

The optimization is an overall throughput improvement: modest and stable in the default scene, large but noisy in the busy scene. It does not yet establish a busy-scene 1% low or tail-latency win.

These measurements are CPU-observed frame intervals around completed Present calls. They do not provide GPU timestamps or total CPU use across worker threads, and Present time is diagnostic evidence rather than proof of reduced engine CPU work.
