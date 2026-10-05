# Stage 3 – System Design, Architecture & Development Setup

## 3.1 High-level architecture

```mermaid
flowchart LR
    subgraph Kernel["Kernel space"]
        DRV["vsensor.ko<br/>misc device /dev/vsensor<br/>read() + ioctl()"]
    end
    subgraph User["User space: sensorguardd"]
        direction TB
        PROD["Producer thread<br/>timerfd + poll"]
        Q[("BoundedQueue&lt;Sample&gt;")]
        CONS["Consumer thread"]
        MON["Monitor<br/>FaultEngine + StateMachine"]
        MAIN["Main thread<br/>signalfd + poll"]
    end
    SIM["SimulatedSource<br/>(no kernel needed)"]
    CTL["sgctl<br/>(ioctl tool)"]
    OUT[("sensorguard.log<br/>samples.csv")]

    DRV -- "read(): vsensor_sample" --> PROD
    SIM -. "alternative source" .-> PROD
    CTL -- "ioctl SET_FAULT" --> DRV
    PROD --> Q --> CONS --> MON
    CONS --> OUT
    MAIN -. "SIGINT/SIGTERM: stop<br/>SIGUSR1: stats" .-> PROD
```

**Data flow:** timer tick → read one sample → push to queue → consumer takes it → each detector votes
→ state machine updates → transition logged / row written to CSV.

## 3.2 Components and responsibilities

| Component | Responsibility |
|---|---|
| `vsensor.c` | Generate readings, hold fault mode, serve `read`/`ioctl`, protect state with a mutex |
| `vsensor_ioctl.h` | Single shared ABI header (kernel + user) – prevents struct/enum mismatch |
| `ISampleSource` | Abstraction: `DeviceSource` (real) and `SimulatedSource` (test) |
| `IDetector` + 6 detectors | One fault type each; stateless interface, internal sliding windows |
| `FaultEngine` | Runs detectors in priority order; first fault wins |
| `StateMachine` | Turns noisy per-sample verdicts into stable health states |
| `Monitor` | Combines engine + state machine + statistics (mutex-protected snapshot) |
| `BoundedQueue<T>` | Thread-safe hand-off, back-pressure, clean shutdown via `close()` |
| `Logger` | Thread-safe timestamped logging to stderr + file |
| `main.cpp` | CLI parsing, thread orchestration, signals |

## 3.3 Data structures

```c
struct vsensor_sample {          /* kernel -> user ABI, 24 bytes */
    __u64 timestamp_ns;          /* ktime_get_ns()                */
    __s32 value_mC;              /* milli-°C (kernel: no floats)  */
    __u32 seq;                   /* monotonically increasing      */
    __u32 flags;                 /* bit0 = VALID                  */
    __u32 reserved;
};
```
```cpp
struct Sample { uint64_t timestamp_ns; double value_c; uint32_t seq; bool valid; };
enum class FaultType { None, Dropout, OutOfRange, StuckAt, Spike, Drift, Noise };
enum class State     { Init, Normal, Suspect, Fault, Recovering };
```
Sliding windows use `std::deque<double>`; queue uses `std::deque<T>` + `std::mutex` + 2 condition variables.

## 3.4 Detection algorithms

| Detector | Algorithm | Default |
|---|---|---|
| Dropout | `valid == false` | – |
| Range | `v < min || v > max` | –40 … 85 °C |
| Stuck-at | run-length of identical values ≥ N | N = 8 |
| Spike | `|v − median(last 5 accepted)| > T`; spikes are **not** added to the window | T = 5 °C |
| Drift | baseline = mean of first 20 samples; flag if `|mean(last 10) − baseline| > T` | T = 3 °C |
| Noise | population std-dev of last 10 samples > T | T = 1 °C |

*Design decision:* detectors run in **priority order and short-circuit**, so a spike cannot also inflate
the noise detector's window.

## 3.5 UML diagrams

### Class diagram
```mermaid
classDiagram
    class ISampleSource {
        <<interface>>
        +read(Sample&) bool
        +setFault(InjectMode) bool
        +describe() string
    }
    class DeviceSource { -fd_ : int
        +reset() bool
        +getFault(InjectMode&) bool }
    class SimulatedSource { -rng_ : uint32
        -fault_ : InjectMode }
    ISampleSource <|-- DeviceSource
    ISampleSource <|-- SimulatedSource

    class IDetector {
        <<interface>>
        +evaluate(Sample) FaultType
        +reset()
        +name() const char*
    }
    class DropoutDetector
    class RangeDetector
    class StuckDetector
    class SpikeDetector
    class DriftDetector
    class NoiseDetector
    IDetector <|-- DropoutDetector
    IDetector <|-- RangeDetector
    IDetector <|-- StuckDetector
    IDetector <|-- SpikeDetector
    IDetector <|-- DriftDetector
    IDetector <|-- NoiseDetector

    class FaultEngine { +process(Sample) FaultType }
    class StateMachine { -state_ : State
        -healthy_ : int
        +update(bool faulty) State }
    class Monitor { +onSample(Sample) Verdict
        +stats() Stats }
    class BoundedQueue~T~ { +push(T) bool
        +pop(T&) bool
        +close() }

    FaultEngine o-- "6" IDetector
    Monitor *-- FaultEngine
    Monitor *-- StateMachine
```

### Sequence diagram – one sample, and a fault being confirmed
```mermaid
sequenceDiagram
    autonumber
    participant T as timerfd
    participant P as Producer thread
    participant D as /dev/vsensor
    participant Q as BoundedQueue
    participant C as Consumer thread
    participant M as Monitor
    participant L as Logger/CSV

    T->>P: tick (poll wakes)
    P->>D: read()
    D-->>P: vsensor_sample
    P->>Q: push(Sample)
    Q->>C: pop(Sample)
    C->>M: onSample(Sample)
    M->>M: FaultEngine.process() → FaultType
    M->>M: StateMachine.update(faulty)
    M-->>C: Verdict{fault, state, transitioned}
    C->>L: write CSV row
    alt state changed
        C->>L: log "STATE NORMAL -> SUSPECT"
    end
```

### State machine diagram
```mermaid
stateDiagram-v2
    [*] --> INIT
    INIT --> NORMAL : 20 consecutive healthy samples
    NORMAL --> SUSPECT : any fault
    SUSPECT --> NORMAL : 5 consecutive healthy samples
    SUSPECT --> FAULT : ≥3 faults in last 10 samples
    FAULT --> RECOVERING : 3 consecutive healthy samples
    RECOVERING --> FAULT : any fault
    RECOVERING --> NORMAL : 10 consecutive healthy samples
```
(A fault during INIT restarts the warm-up counter.)

## 3.6 Implementation plan

1. Shared ABI header + driver skeleton (`insmod`, `/dev/vsensor`, `read`).
2. `ioctl` + fault generation in driver; `SimulatedSource` mirror.
3. Detectors one by one, each with unit tests.
4. State machine, then `Monitor`.
5. Daemon: queue, threads, timerfd, signals, CSV/log.
6. Testing, CI, documentation, demo.

## 3.7 Development environment & tools

| Need | Tool |
|---|---|
| OS | Ubuntu 22.04/24.04 (a **VM** is recommended for kernel work) |
| Compiler / build | `g++` ≥ 9, `cmake` ≥ 3.10, `make` |
| Kernel build | `linux-headers-$(uname -r)`, `build-essential` |
| Debugging | `gdb`, `dmesg -w`, `strace`, `-fsanitize=thread,address` |
| VCS / CI | Git, GitHub Actions |

```bash
sudo apt update
sudo apt install -y build-essential cmake git linux-headers-$(uname -r)
```

## 3.8 Git repository & branching strategy

**Model:** simplified Git-Flow.

```
main      ●────────────●───────────────●────────────●  (tags: stage-N, v1.0.0) – always working
           \          / \             / \          /
develop     ●───●───●   ●────●───●───●   ●──●────●      integration branch
             \ /           \ /            \ /
feature/*     ●             ●              ●            e.g. feature/driver, feature/detectors
```

| Rule | Detail |
|---|---|
| `main` | Stable, tagged at the end of each stage |
| `develop` | Integration; CI must be green |
| `feature/<topic>` | One branch per module; merged by pull request |
| Commits | Conventional style: `feat(driver): add SET_FAULT ioctl`, `test:`, `docs:`, `fix:`, `ci:` |
| Tags | `stage-1` … `stage-6`, `v1.0.0` |

## 3.9 Documentation & progress tracking
- `docs/stageN_*.md` – one document per stage (this folder).
- `docs/PROGRESS.md` – dated progress log: done / issues / next.
- GitHub **Issues** for tasks, **Milestones** = stages, **Project board** = Todo / Doing / Done.

## 3.10 Stage 3 deliverable checklist
- [x] Architecture diagram, component list, data structures
- [x] UML: class, sequence, state machine
- [x] Implementation plan, environment setup, Git strategy
- [x] Tagged `stage-3`
- [ ] **Presentation**: show the 4 diagrams + branching model
- [ ] Roadmap → Stage 4: implement driver + core detection
