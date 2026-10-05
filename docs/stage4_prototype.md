# Stage 4 – Initial Implementation & Prototype

## 4.1 What was implemented
| Milestone | Content | Files |
|---|---|---|
| M1 | Driver: misc device, `read()`, deterministic data generator | `driver/vsensor.c`, `vsensor_ioctl.h` |
| M2 | Driver: `ioctl` fault injection (5 modes) + RESET | same |
| M3 | C++ core: `Sample`, detectors, `FaultEngine` | `detectors.*`, `types.*` |
| M4 | `StateMachine`, `Monitor` | `state_machine.*`, `monitor.*` |
| M5 | Sources (`DeviceSource`, `SimulatedSource`) | `sources.hpp`, `*_source.cpp` |
| M6 | Daemon prototype: threads, timerfd, queue, logger | `main.cpp`, `bounded_queue.hpp`, `logger.*` |

Components were integrated progressively: simulator → detectors → state machine → daemon → real device.

## 4.2 Driver walkthrough (key points to explain in the demo)
- `misc_register()` with `MISC_DYNAMIC_MINOR` creates `/dev/vsensor` automatically (no major/minor bookkeeping).
- `read()` rejects buffers smaller than one sample (`-EINVAL`), fills a **zeroed** struct (no kernel stack leak),
  and uses `copy_to_user()`; returns `-EFAULT` on a bad pointer.
- `ioctl()` validates the fault mode (`-EINVAL`), unknown commands return `-ENOTTY`.
- A **mutex** serialises `read` vs `ioctl` (both touch generator state).
- Floats are not allowed in kernel code → temperature in **milli-°C** (`__s32`).

## 4.3 Running the prototype

```bash
# Build user space
cmake -S . -B build && cmake --build build -j

# A) Without the kernel module
./build/sensorguardd --simulate --period-ms 100 --samples 200 --inject drift:60

# B) With the real driver (use a VM!)
./scripts/load_driver.sh
./build/sgctl /dev/vsensor read 3
./build/sensorguardd -d /dev/vsensor -p 100 &
./build/sgctl /dev/vsensor set stuck      # watch the daemon log the transition
kill -USR1 $!                             # statistics
kill -TERM $!                             # graceful stop
./scripts/unload_driver.sh
```

Example daemon output (spike fault injected at sample 50, cleared at 100):
```
sensorguardd started: source=simulated period=1ms
STATE INIT -> NORMAL (seq=19, fault=none, value=25.09C)
[inject] fault 'spike' enabled at sample 50
STATE NORMAL -> SUSPECT (seq=52, fault=spike, value=49.86C)
STATE SUSPECT -> FAULT (seq=58, fault=spike, value=49.86C)
[inject] fault cleared at sample 100
```

## 4.4 Development log: issues and solutions

| # | Issue | Solution |
|---|---|---|
| 1 | Kernel code cannot use `float` | Driver reports milli-°C integers; user space converts to `double` |
| 2 | Kernel/user enums could silently diverge | Single shared `vsensor_ioctl.h` + `static_assert`s in `device_source.cpp` |
| 3 | A spike also inflated the *noise* detector's window → wrong fault label | Detectors short-circuit in priority order; spikes are not added to the spike median window |
| 4 | Isolated spikes (1 in 5) never accumulated enough faults to reach FAULT | Confirmation uses "≥3 faults in last 10 samples" instead of "3 in a row"; spike period set to every 3rd sample |
| 5 | Debugging the driver without risking the host | Bit-exact `SimulatedSource` lets all logic be developed/tested in user space first |
| 6 | Shutting down threads cleanly | `eventfd` wakes producer's `poll`; `BoundedQueue::close()` lets consumer drain then exit |

## 4.5 Known limitations at the end of Stage 4
- Unit tests only for detectors (full suite arrives in Stage 5).
- Thresholds are hard-coded defaults.
- Drift baseline assumes a healthy start.

## 4.6 Stage 4 deliverable checklist
- [x] Core modules implemented, prototype runs
- [x] Progress/issues/solutions recorded (above + `PROGRESS.md`)
- [x] Tagged `stage-4`
- [ ] **Demo**: run 4.3 live (injection → detection)
- [ ] Roadmap → Stage 5: tests, hardening, CI
