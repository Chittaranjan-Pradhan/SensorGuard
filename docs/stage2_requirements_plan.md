# Stage 2 – Project Requirements Document (PRD) & Development Plan

**Project:** SensorGuard  **Version:** 1.0  **Author:** <YOUR NAME>

## 2.1 Purpose
Specify what SensorGuard must do and how well, so design and tests can be traced back to requirements.

## 2.2 Stakeholders / users
- **Operator** – runs the daemon and reads logs/CSV.
- **Developer/tester** – injects faults to validate the system.

## 2.3 Functional requirements

| ID | Requirement | Priority |
|---|---|---|
| FR-1 | The driver shall expose `/dev/vsensor` and return one `struct vsensor_sample` per `read()`. | Must |
| FR-2 | The driver shall support `ioctl` SET_FAULT / GET_FAULT / RESET. | Must |
| FR-3 | The driver shall be able to simulate: stuck, spike, drift, noise, dropout. | Must |
| FR-4 | The daemon shall sample at a configurable period (default 100 ms). | Must |
| FR-5 | The daemon shall detect: dropout, out-of-range, stuck-at, spike, drift, noise. | Must |
| FR-6 | The daemon shall maintain a health state: INIT, NORMAL, SUSPECT, FAULT, RECOVERING. | Must |
| FR-7 | The daemon shall log every state transition with time, sequence number, fault type, value. | Must |
| FR-8 | The daemon shall optionally write a per-sample CSV (seq, timestamp, value, valid, fault, state). | Should |
| FR-9 | The daemon shall shut down gracefully on SIGINT/SIGTERM and print statistics on SIGUSR1. | Must |
| FR-10 | The daemon shall allow scripted fault injection (`--inject MODE:START[:END]`). | Should |
| FR-11 | The daemon shall run without the kernel module using a built-in simulator (`--simulate`). | Should |
| FR-12 | The daemon shall return exit code 2 if the sensor ends in FAULT. | Could |
| FR-13 | A control tool (`sgctl`) shall allow manual fault injection and reads. | Could |

## 2.4 Non-functional requirements

| ID | Requirement | Measure |
|---|---|---|
| NFR-1 | **Correctness** – no false alarms on a healthy signal | 0 faults in 25 seeds × 600 samples |
| NFR-2 | **Detection latency** – injected fault reaches FAULT quickly | ≤ 60 samples for all fault types (drift is the slowest) |
| NFR-3 | **Robustness** – driver rejects bad input | tiny read buffer → `EINVAL`; unknown ioctl → `ENOTTY`; bad mode → `EINVAL` |
| NFR-4 | **Thread safety** | no data races under ThreadSanitizer; driver state protected by mutex |
| NFR-5 | **Portability** | builds with GCC ≥ 9, C++17, CMake ≥ 3.10; kernel ≥ 5.x |
| NFR-6 | **Maintainability** | warnings-clean with `-Wall -Wextra -Wpedantic`; modular classes; documented |
| NFR-7 | **Testability** | automated unit, integration, E2E tests run in CI |
| NFR-8 | **Resource use** | bounded memory (queue capacity 256); no busy-waiting (poll/condvar) |

## 2.5 Constraints and assumptions
- Linux only (uses `timerfd`, `signalfd`, `eventfd`, kernel module).
- Kernel headers needed to build the module; root needed to load it.
- Assumption: the sensor is healthy during the first 20 samples (drift baseline is learned there).

## 2.6 Modules and deliverables

| Module | Files | Deliverable |
|---|---|---|
| M1 Kernel driver | `driver/` | `vsensor.ko` |
| M2 Detectors | `detectors.*` | six detector classes + `FaultEngine` |
| M3 State machine | `state_machine.*` | 5-state health machine |
| M4 Monitor | `monitor.*` | engine + SM + statistics |
| M5 Sources | `sources.hpp`, `device_source.cpp`, `simulated_source.cpp` | device + simulator |
| M6 Daemon | `main.cpp`, `bounded_queue.hpp`, `logger.*` | `sensorguardd` |
| M7 Tools/tests | `sgctl.cpp`, `tests/`, `scripts/` | `sgctl`, `sg_tests`, E2E scripts |
| M8 Documentation | `docs/` | PRD, design, UML, test report, final report |

## 2.7 Development plan and timeline (6 stages)

| Stage | Focus | Output | Git tag |
|---|---|---|---|
| 1 | Introduction | idea, scope | `stage-1` |
| 2 | Requirements & plan | this PRD | `stage-2` |
| 3 | Design & architecture + environment | diagrams, UML, repo workflow, build system | `stage-3` |
| 4 | Initial implementation & prototype | driver + core detection working | `stage-4` |
| 5 | Testing, integration & improvement | full test suite, CI, bug fixes | `stage-5` |
| 6 | Final delivery | report, demo, release | `stage-6`, `v1.0.0` |

**Risks**

| Risk | Mitigation |
|---|---|
| Kernel headers unavailable on the dev machine | Simulator mirrors the driver; CI builds module separately |
| Driver crash (kernel panic) during development | Develop in a VM; keep driver tiny; test via simulator first |
| Thresholds cause false alarms | Statistical tests over many seeds (NFR-1) |

## 2.8 Requirement → test traceability

| Requirement | Verified by |
|---|---|
| FR-1, FR-2 | `scripts/test_driver.sh` (DRV-1) |
| FR-3 | `sim_*` tests, DRV-2 |
| FR-4, FR-7, FR-8 | E2E-1 … E2E-3 |
| FR-5 | `test_detectors.cpp`, `detects_*_fault` |
| FR-6 | `test_state_machine.cpp` |
| FR-9 | E2E-4 |
| FR-10, FR-12 | E2E-2, E2E-3 |
| FR-11 | all simulator-based tests |
| NFR-1 | `no_false_positives_on_healthy_signal_many_seeds` |
| NFR-2 | `detects_*_fault` (≤150 samples bound; measured values in Stage 5) |
| NFR-3 | DRV-1 (`dd bs=4`) |
| NFR-4 | queue tests + TSan run (Stage 5) |

## 2.9 Stage 2 deliverable checklist
- [x] Functional + non-functional requirements
- [x] Scope, modules, deliverables
- [x] Development plan, timeline, risks
- [x] Git commit tagged `stage-2`
- [ ] **Presentation**: requirements table + timeline
- [ ] Roadmap → Stage 3: architecture, UML, environment
