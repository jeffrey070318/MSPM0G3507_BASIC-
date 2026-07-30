# VS Code Local Configuration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Prevent personal Windows tool paths from dirtying shared branches while retaining ready-to-copy VS Code and CMake examples.

**Architecture:** Git tracks portable tasks and example files only. Each developer keeps ignored local VS Code files and `CMakeUserPresets.json`, while shared `CMakePresets.json` contains path-free base presets.

**Tech Stack:** VS Code Cortex-Debug, CMake presets, PowerShell, Git.

---

### Task 1: Split Shared And Local Configuration

**Files:**
- Rename: `.vscode/launch.json` to `.vscode/launch.example.json`
- Rename: `.vscode/settings.json` to `.vscode/settings.example.json`
- Rename: `.vscode/c_cpp_properties.json` to `.vscode/c_cpp_properties.example.json`
- Create: `.vscode/README.md`

- [ ] Keep CMSIS-DAP and XDS110 launch definitions in the example.
- [ ] Restore ignored local copies so the current developer can continue debugging.
- [ ] Document the one-time copy process for new checkouts.

### Task 2: Make Tasks And Presets Portable

**Files:**
- Modify: `.vscode/tasks.json`
- Modify: `CMakePresets.json`
- Create: `CMakeUserPresets.example.json`

- [ ] Replace absolute CMake executable paths with the `cmake` command.
- [ ] Configure and build through `local` and `local-no-rtos` presets.
- [ ] Keep only path-free base presets in `CMakePresets.json`.
- [ ] Put SDK, SysConfig, Ninja, OpenOCD and PATH values in the user preset example.

### Task 3: Verify And Integrate

**Files:**
- Test: all modified JSON files and CMake preset discovery
- Test: `tests/run_host_tests.ps1`
- Test: FreeRTOS and NoRTOS CMake builds

- [ ] Parse every tracked JSON file.
- [ ] Confirm `cmake --list-presets=all` sees both shared and local presets.
- [ ] Run all host tests and both firmware builds.
- [ ] Commit on the feature branch, fetch remote, fast-forward `master`, push, and verify hashes.
