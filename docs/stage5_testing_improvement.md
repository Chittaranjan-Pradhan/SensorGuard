# Stage 5 – Testing, Integration & Improvement

## 5.1 Test strategy

| Level | What | Tool | Location |
|---|---|---|---|
| Unit | Each detector, state machine, queue, simulator | custom micro-framework (`TEST`, `CHECK`) | `tests/test_*.cpp` |
| Integration | Simulator → Monitor, all 5 faults + recovery + false-positive study | same | `tests/test_monitor.cpp` |
| System / E2E | Real `sensorguardd` binary: CSV, exit codes, signals, bad args | bash | `scripts/e2e_sim.sh` |
| Driver | Real kernel module: read, ioctl, EINVAL paths, daemon on device | bash | `scripts/test_driver.sh` |
| Concurrency | Data-race check | `-fsanitize=thread` | see 5.4 |
| CI | Everything on every push | GitHub Actions | `.github/workflows/ci.yml` |

Run everything: `make test`  (or `ctest --output-on-failure` in `build/`).

## 5.2 Results

| Suite | Result |
|---|---|
| Unit + integration (`sg_tests`) | **43 / 43 passed** |
| End-to-end (`e2e_sim.sh`) | **13 / 13 checks passed** (5 scenarios) |
| Compiler | 0 warnings with `-Wall -Wextra -Wpedantic` (GCC 13.3) |
| ThreadSanitizer (daemon, drift scenario) | **0 data-race reports** |
| Driver tests (`test_driver.sh`) | run on your machine after `load_driver.sh` – paste output below |

```
<paste output of ./scripts/test_driver.sh here>
```

## 5.3 Measured quality (NFR verification)

**NFR-1 – false alarms.** 1,000 seeds × 1,000 healthy samples = **1,000,000 samples → 0 false faults**.

**NFR-2 – detection latency.** 50 seeds per fault, samples counted from the moment of injection:

| Injected fault | Detected as | First SUSPECT (worst) | FAULT reached (avg / worst) | At 100 ms period |
|---|---|---|---|---|
| Dropout | `dropout` | 1 | 3.0 / 3 | ≤ 0.3 s |
| Noise | `noise` | 7 | 4.7 / 9 | ≤ 0.9 s |
| Stuck | `stuck_at` | 7 | 9.0 / 9 | ≤ 0.9 s |
| Spike | `spike` | 3 | 9.0 / 9 | ≤ 0.9 s |
| Drift (0.1 °C/sample) | `drift` | 37 | 38.0 / 39 | ≤ 3.9 s |

Target was ≤ 60 samples → **met** (worst case 39).

## 5.4 Defects, design issues and fixes

Issues below were resolved while building the test suite; each is now guarded by a regression test.
**Add your own real findings to this table as you test on your machine** (especially driver bugs found
via `dmesg`) – the process evidence is part of the grade.

| # | Issue | Guarded by test | Fix |
|---|---|---|---|
| 1 | "3 faults in a row" can never confirm an intermittent fault (e.g. a spike every 3rd sample) | `sm_every_third_sample_faulty_still_confirms` | sliding-window confirmation (Stage 4 #4) |
| 2 | A spike inflating the noise window would mislabel the fault type | `detects_spike_fault`, `detects_noise_fault` (10 seeds each) | priority short-circuit in `FaultEngine` |
| 3 | Daemon must flush the CSV and exit cleanly on SIGTERM | E2E-4 | consumer drains queue and flushes before exit |
| 4 | Driver must not leak kernel stack bytes through `reserved` | review checklist (verify with `sgctl read`) | `memset(s, 0, sizeof *s)` before filling |
| 5 | <your finding> | | |

## 5.5 Improvements made
- **Performance:** detectors are O(window) with windows ≤ 10; the consumer thread uses a blocking queue (no polling) → negligible CPU.
- **Reliability:** bounded queue back-pressure; overrun detection (timerfd tick count) logged as warning; graceful shutdown.
- **Code quality:** pure-virtual interfaces, RAII file descriptors, warnings-clean build, shared ABI header, CI.

## 5.6 Stage 5 deliverable checklist
- [x] Unit, integration, system tests; bugs fixed
- [x] Documentation + Git updated, tagged `stage-5`
- [ ] **Presentation**: test pyramid, results table, bug list
- [ ] Roadmap → Stage 6: final report, demo rehearsal, release `v1.0.0`
