# SensorGuard – Smart Sensor Fault Detection System

## Project Overview

SensorGuard is a Linux-based smart sensor monitoring system developed using C++ and a Linux virtual sensor driver.

The project simulates a temperature sensor and monitors sensor readings to identify abnormal or faulty behavior. When a fault is detected, the system classifies the fault, updates the sensor health state, and records the event for further analysis.

The project demonstrates how C++, Linux system programming, device drivers, and embedded-system concepts can work together in a sensor monitoring application.

---

## Problem Statement

Sensors used in embedded and industrial systems may produce incorrect or abnormal readings because of communication failures, sudden spikes, stuck values, drift, or excessive noise.

If these abnormal readings are not detected, they can lead to incorrect decisions or system failures.

SensorGuard addresses this problem by continuously monitoring sensor data and detecting different types of sensor faults.

---

## Objectives

The main objectives of SensorGuard are:

- Simulate a temperature sensor in a Linux environment.
- Provide sensor data through a virtual Linux device.
- Monitor sensor readings using a C++ application.
- Detect different types of sensor faults.
- Classify detected faults.
- Maintain the health state of the sensor.
- Record detected events for analysis.
- Demonstrate interaction between user-space software and a Linux device driver.

---

## How SensorGuard Works

The system follows the general workflow:

**Sensor → Data Source → Monitoring → Fault Detection → Health State → Logging → Analysis**

The virtual sensor generates temperature readings.

The C++ monitoring application reads these values and passes them through different fault detectors.

The detectors check the readings for abnormal conditions such as:

- Dropout
- Out-of-range values
- Stuck-at values
- Sudden spikes
- Drift
- Excessive noise

When a fault is detected, the sensor state is updated and the event is recorded for further analysis.

---

## System Architecture

```text
                ┌──────────────────────┐
                │   Virtual Sensor     │
                │   Linux Driver       │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │   Sensor Data Source │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │   Sensor Monitor     │
                │       (C++)          │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │   Fault Detectors    │
                ├──────────────────────┤
                │ Dropout              │
                │ Range                │
                │ Stuck                │
                │ Spike                │
                │ Drift                │
                │ Noise                │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │   Sensor State       │
                │   State Machine      │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │    Event Logger      │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │   Fault Analysis     │
                └──────────────────────┘
