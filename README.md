SensorGuard

Smart Sensor Fault Detection & Event Analysis System

SensorGuard is a Linux-based smart sensor monitoring and
fault-detection system developed mainly in C++. It uses a virtual
temperature sensor to generate controlled readings, detects abnormal
sensor behavior, tracks sensor health through a state machine, and
records events for later analysis.

Core workflow: Read → Monitor → Detect → Classify → Track Health →
Recover → Log → Analyze

1. Project Overview

In embedded, industrial, environmental, and IoT systems, sensor readings
can become unreliable. A sensor may stop responding, produce an
out-of-range value, remain stuck, suddenly spike, gradually drift, or
become excessively noisy.

SensorGuard demonstrates how a monitoring system can continuously
observe sensor data, detect abnormal behavior, track sensor health,
record events, and verify recovery.

The current implementation uses a simulated temperature sensor, so
the project can be tested without physical hardware.

2. Problem Statement

A system should not blindly trust sensor readings. Faulty readings can
cause incorrect decisions in a larger embedded or industrial system.

SensorGuard addresses this problem by monitoring sensor data and
identifying abnormal behavior such as:

Dropout

Out-of-range values

Stuck-at values

Spikes

Drift

Noise

The system also maintains a health state, records important events, and
produces data that can be analyzed later.

3. Objectives

Build a Linux-based sensor monitoring system.

Generate controlled temperature sensor data without physical
hardware.

Detect multiple types of sensor faults.

Track sensor health using a state machine.

Use modular C++ fault detectors.

Record fault and state-transition events.

Generate per-sample data for analysis.

Demonstrate Linux user-space and device-driver concepts.

Provide automated unit, integration, and end-to-end testing.

Use Git, GitHub, and CI for development and verification.

4. System Architecture

Virtual Temperature Sensor
            │
            ▼
Linux Sensor Interface / Virtual Device
            │
            ▼
C++ SensorGuard Monitor
            │
            ▼
     Fault Detectors
 ┌──────────┼───────────┐
 ▼          ▼           ▼
Dropout   Range     Stuck-at
Spike     Drift       Noise
            │
            ▼
      Health State Machine
            │
            ▼
 INIT → NORMAL → SUSPECT → FAULT
                              │
                              ▼
                         RECOVERING
                              │
                              ▼
                           NORMAL
                         /                                ▼          ▼
                   Event Log   Fault Analysis
                   demo.log     demo.csv

5. Main Components

Virtual Sensor

Generates temperature readings and controlled fault conditions. This
allows repeatable testing without physical hardware.

Linux Virtual Sensor Driver

The project contains:

driver/vsensor.c

The driver demonstrates a Linux device interface and exposes the virtual
sensor as:

/dev/vsensor

The application can also run in simulation mode.

C++ Monitoring Application

The C++ application reads sensor data, invokes fault detectors, manages
health states, and records events.

Fault Detectors

Separate detector modules identify different abnormal behaviors. This
keeps the detection logic modular and easier to test.

Health State Machine

The sensor health is represented by:

INIT → NORMAL → SUSPECT → FAULT → RECOVERING → NORMAL

6. Sensor Fault Types

6.1 Dropout

The sensor stops providing a valid reading.

25.1
25.0
25.2
----
----
----

6.2 Out-of-Range

The sensor produces a value outside the defined acceptable range.

25°C
26°C
25°C
150°C  ← abnormal

6.3 Stuck-at

The sensor repeatedly reports the same value.

25.1
25.1
25.1
25.1
25.1

6.4 Spike

The sensor suddenly changes to an abnormal value.

25°C
25°C
25°C
50°C  ← sudden spike
50°C
25°C

6.5 Drift

The sensor gradually moves away from expected behavior.

25°C
26°C
27°C
28°C
29°C
30°C

6.6 Noise

The sensor produces excessive irregular variation.

25.1
28.7
23.4
29.2
24.0
27.8

7. Health State Machine

State                               Meaning

INIT                              System has started and is
establishing the initial condition.

NORMAL                            Sensor readings are behaving
normally.

SUSPECT                           Abnormal behavior has been observed
and needs confirmation.

FAULT                             A sensor fault has been confirmed.

Example

INIT
  ↓
NORMAL
  ↓
SUSPECT
  ↓
FAULT
  ↓
RECOVERING
  ↓
NORMAL

This means:

Healthy → abnormal behavior → confirmed fault → recovery verification
→ healthy

8. Event Logging

SensorGuard records important events during execution.

demo.log

Contains events such as:

Fault detection

Fault injection

Fault clearing

Health-state transitions

Recovery

Final statistics

demo.csv

Contains per-sample sensor data for later analysis.

This allows the system to preserve both important events and
raw/per-sample information.

9. Fault Analysis

The recorded data can be used to determine:

Which fault occurred

When it occurred

What sensor values were produced

How the health state changed

When the fault was cleared

Whether the system returned to normal

Demonstrated spike scenario

Normal sensor readings
        ↓
Spike injected
        ↓
Spike detected
        ↓
NORMAL → SUSPECT
        ↓
SUSPECT → FAULT
        ↓
Spike cleared
        ↓
FAULT → RECOVERING
        ↓
RECOVERING → NORMAL

10. Demonstration Result

A successful spike demonstration produced:

Samples processed : 200
State transitions : 5
Spike detections  : 16
Final state       : NORMAL

The five transitions were:

INIT → NORMAL
NORMAL → SUSPECT
SUSPECT → FAULT
FAULT → RECOVERING
RECOVERING → NORMAL

The demonstration also produced:

demo.log
demo.csv

Occasional sampling overrun / ticks missed warnings may appear
when running inside VirtualBox. These are timing/scheduling warnings
from the virtualized environment and are separate from the
sensor-fault detection logic.

11. Project Structure

SensorGuard/
├── .github/
│   └── workflows/
│       └── ci.yml
├── driver/
│   └── vsensor.c
├── include/
├── src/
│   ├── detectors.cpp
│   ├── monitor.cpp
│   ├── simulated_source.cpp
│   ├── device_source.cpp
│   ├── state_machine.cpp
│   ├── logger.cpp
│   ├── sgctl.cpp
│   └── main.cpp
├── tests/
├── scripts/
│   ├── e2e_sim.sh
│   ├── run_demo.sh
│   ├── load_driver.sh
│   ├── unload_driver.sh
│   ├── test_driver.sh
│   └── create_stage_history.sh
├── docs/
├── CMakeLists.txt
├── Makefile
├── README.md
└── LICENSE

12. Technologies Used

Technology                 Purpose

C++                    Monitoring, fault detection, state management
C                      Linux virtual device driver
Linux                  System-programming environment
Linux Kernel Modules   Virtual sensor/device interface
CMake                  Build configuration
Make                   Build automation
Bash                   Demo and testing scripts
Git                    Version control
GitHub                 Repository and CI
VirtualBox             Ubuntu virtual-machine environment

13. Build and Run

Prerequisites

Linux environment with:

g++

cmake

make

git

Build

From the project root:

make

Run the demonstration

chmod +x scripts/run_demo.sh
./scripts/run_demo.sh

Run end-to-end tests

chmod +x scripts/e2e_sim.sh
./scripts/e2e_sim.sh

Expected result:

E2E: ALL PASSED

14. Testing and CI

The project includes automated testing for:

Unit behavior

Integration behavior

End-to-end simulation

Fault injection

Recovery behavior

Graceful shutdown

Invalid input handling

GitHub Actions performs the automated pipeline:

Configure
   ↓
Build
   ↓
Kernel Module Build
   ↓
Unit / Integration Tests
   ↓
E2E Tests

The latest CI run successfully passes these stages.

15. Linux and Embedded Concepts Demonstrated

Linux

Command line

Processes

File permissions

User-space and kernel-space concepts

System-level monitoring

C++

Object-oriented design

Modular classes

Data processing

State management

File handling

Error handling

Device Drivers

Linux kernel module

Virtual device

Device interface

User-space/device interaction

System Programming

Process execution

Signals

Periodic monitoring

File operations

System-level interaction

Software Engineering

Modular architecture

Automated testing

CMake and Make

Git version control

GitHub Actions

Documentation

16. Limitations

The current sensor is simulated rather than connected to physical
hardware.

Fault-detection rules are predefined.

The project does not currently use machine learning for anomaly
detection.

The demonstration focuses on a virtual sensor.

Timing behavior can be affected by the VirtualBox environment.

17. Future Enhancements

Connect a real temperature sensor.

Add I2C/SPI sensor interfaces.

Support multiple sensors.

Add a graphical monitoring dashboard.

Store historical fault statistics.

Make detection thresholds configurable.

Improve timing and scheduling.

Add adaptive or machine-learning-based anomaly detection.

Add alerts for critical faults.

Integrate additional embedded hardware.

18. Possible Applications

SensorGuard's approach can be adapted for:

Industrial equipment monitoring

IoT systems

Environmental monitoring

Temperature monitoring

Embedded diagnostics

Predictive maintenance

Machine health monitoring

Automation systems

19. Key Project Idea

SensorGuard does not blindly trust sensor data. It continuously
monitors the data, detects abnormal behavior, determines sensor
health, records important events, and verifies recovery.

The project combines this idea with Linux, C++, device-driver concepts,
system programming, automated testing, and Git-based development.

20. Conclusion

SensorGuard demonstrates a complete sensor-monitoring workflow using a
controlled Linux environment.

The system can generate sensor data, identify different abnormal
behaviors, track health states, record events, and verify that the
sensor returns to a normal state after a demonstrated fault condition.

The project brings together:

Sensor Simulation + C++ + Linux + Device Drivers + Fault Detection +
State Machine + Event Logging + Testing + GitHub CI

Author

Chittaranjan Pradhan

Project: SensorGuard -- Smart Sensor Fault Detection & Event
Analysis System

Repository: Chittaranjan-Pradhan/SensorGuard
