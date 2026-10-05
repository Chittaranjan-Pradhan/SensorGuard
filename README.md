# SensorGuard – Smart Sensor Fault Detection

[![CI](https://github.com/<YOUR_USER>/sensorguard/actions/workflows/ci.yml/badge.svg)](https://github.com/<YOUR_USER>/sensorguard/actions)

A Linux project combining a **kernel device driver**, **system programming**, and **modern C++** to detect
faults (stuck, spike, drift, noise, dropout, out-of-range) in a sensor data stream in real time.

```
 /dev/vsensor  ──read()──►  producer ─► BoundedQueue ─► consumer ─► Monitor(detectors + state machine)
 (vsensor.ko)   ◄─ioctl─     (timerfd+poll)                          └─► log + CSV      INIT→NORMAL→SUSPECT→FAULT→RECOVERING
```

## Quick start (no kernel module needed)

```bash
sudo apt install -y build-essential cmake
make test                     # build + 43 unit/integration tests + E2E scenarios
./build/sensorguardd --simulate --period-ms 100 --samples 200 --inject spike:60:120
```

## With the real kernel driver (use a VM)

```bash
sudo apt install -y linux-headers-$(uname -r)
./scripts/load_driver.sh                  # builds + insmod, creates /dev/vsensor
./build/sgctl /dev/vsensor read 3         # raw samples
./build/sensorguardd -d /dev/vsensor &    # start monitoring
./build/sgctl /dev/vsensor set drift      # inject a fault -> watch the log
kill -USR1 %1                             # print statistics
./scripts/test_driver.sh                  # driver system tests
./scripts/unload_driver.sh
```

## Daemon options

| Option | Meaning |
|---|---|
| `-s, --simulate` | use built-in simulator instead of the driver |
| `-d, --device PATH` | device node (default `/dev/vsensor`) |
| `-p, --period-ms N` | sampling period (default 100) |
| `-n, --samples N` | stop after N samples |
| `-i, --inject MODE:START[:END]` | `stuck|spike|drift|noise|dropout`, e.g. `drift:60` or `spike:50:120` |
| `-c, --csv FILE` / `-l, --log FILE` | per-sample CSV / log file |
| `-q, --quiet` | less verbose |

Signals: `SIGINT/SIGTERM` graceful stop · `SIGUSR1` statistics. Exit code `2` = sensor ended in FAULT.

## Repository layout

```
driver/      vsensor.c, vsensor_ioctl.h (shared ABI), Makefile      ← Linux device driver
include/     sensorguard/*.hpp                                       ← C++ interfaces
src/         detectors, state machine, monitor, sources, daemon      ← C++ / system programming
tests/       unit + integration tests                                ← 43 tests
scripts/     load/unload driver, E2E + driver tests, demo
docs/        stage1 … stage6 documents, PRD, UML (Mermaid), progress log
.github/     CI workflow
```

## Course mapping (Capstone, 6 hours → 6 stages)

| Hour | Stage | Document |
|---|---|---|
| 1 | Introduction + requirements | [`stage1`](docs/stage1_introduction.md), [`stage2`](docs/stage2_requirements_plan.md) |
| 2–3 | System design & architecture, UML | [`stage3`](docs/stage3_design_architecture.md) |
| 4–5 | Implementation planning, environment, prototype | [`stage4`](docs/stage4_prototype.md) |
| 6 | Progress review, testing, next steps | [`stage5`](docs/stage5_testing_improvement.md), [`stage6`](docs/stage6_final_report.md) |

Progress log: [`docs/PROGRESS.md`](docs/PROGRESS.md)

## License
Driver: GPL-2.0 · everything else: MIT (see `LICENSE`).
