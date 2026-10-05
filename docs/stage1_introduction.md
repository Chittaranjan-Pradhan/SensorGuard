# Stage 1 – Project Introduction

## 1.1 Project title
**SensorGuard – Smart Sensor Fault Detection for Linux**

## 1.2 Idea and objective
Industrial and IoT systems trust their sensors blindly. A temperature sensor that silently freezes,
drifts, or spikes can cause wrong decisions (overheating machines, spoiled cold-chain goods, false alarms).

SensorGuard is a small Linux software stack that **reads a sensor through a kernel driver, detects
faults in real time, and reports the sensor's health**:

| Layer | Technology | Responsibility |
|---|---|---|
| Kernel | **Linux device driver** (`vsensor.ko`, misc char device) | Produces sensor readings; can inject faults via `ioctl` so the system is testable |
| User space | **System programming** (threads, `poll`, `timerfd`, `signalfd`, `eventfd`, `ioctl`, file I/O) | Periodic sampling, signal handling, logging, CSV output |
| Application | **C++17** (OOP, STL, RAII, templates) | Detectors, health state machine, monitor, bounded queue |

## 1.3 Problem to be solved
> *Given a stream of temperature samples, decide automatically – and quickly – whether the sensor is
> healthy, and tell the operator what kind of fault it has.*

Fault types handled:

| Fault | What it looks like |
|---|---|
| **Dropout** | Sensor returns "no data" |
| **Out-of-range** | Value outside the physical limits (–40…85 °C) |
| **Stuck-at** | The same value repeated many times |
| **Spike** | Isolated large outliers |
| **Drift** | Slow, steadily growing offset from the real value |
| **Noise** | Variance far above normal |

## 1.4 Scope
**In scope**
- A virtual sensor kernel driver with fault injection (no special hardware needed).
- A C++ daemon that detects the six faults above and runs a 5-state health machine.
- CLI tools, logging, CSV export, automated tests, CI, full documentation, UML.

**Out of scope** (listed as future work): real hardware (I²C/SPI) drivers, networking/cloud upload,
machine-learning detectors, GUI dashboard.

## 1.5 Expected outcome and applications
- Working driver + daemon demonstrated live: healthy → fault injected → detected → fault cleared → recovery.
- Applications: factory monitoring, cold-chain logistics, smart-home HVAC, automotive ECU self-checks,
  predictive maintenance.

## 1.6 Why this project fits the course
| Course topic | Where it appears |
|---|---|
| Linux device drivers | `driver/vsensor.c`: misc device, `file_operations`, `copy_to_user`, `ioctl`, mutex, module params |
| System programming | `src/main.cpp`: pthreads/`std::thread`, `timerfd`, `poll`, `signalfd`, `eventfd`, `getopt_long`, file I/O |
| C++ | `include/sensorguard/*.hpp`: interfaces, polymorphism, RAII, templates, STL containers |

## 1.7 Stage 1 deliverable checklist
- [x] Idea, objective, problem statement (this document)
- [x] Scope and expected outcome
- [x] Git repository created, first commit tagged `stage-1`
- [ ] **Presentation**: 5 slides – problem, idea, layers, scope, plan
- [ ] Roadmap for next stage → write the PRD (`stage2_requirements_plan.md`)
