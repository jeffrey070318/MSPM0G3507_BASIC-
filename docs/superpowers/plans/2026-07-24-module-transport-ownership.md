# Module Transport Ownership Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Hide I2C and USART BSP registration behind the IMU and VOFA modules.

**Architecture:** Each fixed board module owns one internal BSP instance. Applications initialize and use semantic module APIs without BSP handles.

**Tech Stack:** C11, MSPM0 DriverLib compatibility BSP, CMake/GCC, host-side stubs.

---

### Task 1: Add Failing Ownership Tests

- [ ] Add host stubs for module-owned I2C and USART registration.
- [ ] Test parameterless IMU/VOFA initialization and VOFA JustFloat framing.
- [ ] Run the host suite and confirm it fails against the old public APIs.

### Task 2: Move Registration Into Modules

- [ ] Replace public BSP configuration and handle parameters with singleton module APIs.
- [ ] Keep I2C and USART types in implementation files only.
- [ ] Update module documentation.

### Task 3: Remove App BSP Usage

- [ ] Update `app/robot.c` and UART hardware test to initialize and call modules directly.
- [ ] Search app sources for IMU/VOFA BSP registration residue.

### Task 4: Verify

- [ ] Run all host tests.
- [ ] Build normal and forced-link firmware.
- [ ] Run SysConfig static validation and `git diff --check`.
