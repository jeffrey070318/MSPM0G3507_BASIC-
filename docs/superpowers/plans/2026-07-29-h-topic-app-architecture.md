# H Topic App Architecture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the RM-style app template with a safe, runnable H-topic app skeleton for line following, differential chassis control, camera feedback, stepper balance control, and competition coordination.

**Architecture:** Each app exports only `Init` and `Task`. `robot.c` owns typed command/status snapshots and calls the apps from one 1 ms task; ball balance runs every tick while line follow, competition, and chassis run every 5 ticks. Unknown vision framing and hardware tuning remain explicitly inactive instead of being guessed.

**Tech Stack:** C11 host tests, MSPM0 DriverLib/SysConfig, FreeRTOS, CMake/Arm GNU Toolchain.

---

## File Map

- Create `app/line_follow/line_follow.h/.c`: gray-sensor ownership, line validity, A-marker edge event, line PID output.
- Create `app/ball_balance/ball_balance.h/.c`: vision/stepper ownership, 1 ms stepper service, safe inactive vision boundary.
- Create `app/competition/competition.h/.c`: key handling, table-driven state machine, command generation.
- Modify `app/chassis/chassis.h/.c`: command/status `Task` API; remove manual globals and message-center path.
- Modify `app/robot.c`, `app/robot_task.h`, `app/robot_def.h`: app wiring, timing, tuning sections, OLED snapshot display.
- Modify `app/hardware_test/hardware_test_config.h`: restore normal firmware mode.
- Delete `app/cmd/*`: remove replaced A/B template.
- Modify `tests/run_host_tests.ps1` and add focused app tests.

### Task 1: Chassis Init/Task API

**Files:**
- Modify: `app/chassis/chassis.h`
- Modify: `app/chassis/chassis.c`
- Modify: `tests/chassis_registration_test.c`

- [ ] **Step 1: Change the host test to use typed commands**

Test the disabled command, differential conversion, status output, and return to disabled:

```c
Chassis_Status_t status = {0};
Chassis_Command_t command = {0};
assert(ChassisInit());
ChassisTask(&command, 0.005f, &status);
assert(!status.enabled);

command = (Chassis_Command_t) {
    .vx_mps = 0.2f,
    .wz_radps = 0.1f,
    .enabled = true,
};
ChassisTask(&command, 0.005f, &status);
assert(status.enabled);
```

- [ ] **Step 2: Run the focused test and verify compile failure**

Run: compile the `chassis_registration_test` entry from `tests/run_host_tests.ps1`.

Expected: FAIL because `Chassis_Command_t` and the new `ChassisTask` signature do not exist.

- [ ] **Step 3: Implement the minimal typed API**

Define only:

```c
bool ChassisInit(void);
void ChassisTask(const Chassis_Command_t *command,
    float dt_seconds, Chassis_Status_t *status);
```

Keep DRV8701E registration and differential conversion. Delete `chassis_manual_*`, `ChassisSetManualCommand`, `ChassisDisableManualCommand`, message-center registration, and DWT-owned delta time. Null/disabled commands stop both motors; enabled commands set wheel targets and call `Motor_Update` only for positive `dt_seconds`.

- [ ] **Step 4: Run the focused test**

Expected: `PASS chassis_registration_test`.

- [ ] **Step 5: Commit**

```text
refactor: 收敛底盘为Init和Task接口
```

### Task 2: Line Follow App

**Files:**
- Create: `app/line_follow/line_follow.h`
- Create: `app/line_follow/line_follow.c`
- Create: `tests/line_follow_test.c`
- Create: `tests/line_follow_stubs/gray_sensor.h`
- Create: `tests/line_follow_stubs/ti_msp_dl_config.h`
- Modify: `tests/run_host_tests.ps1`
- Modify: `app/robot_def.h`

- [ ] **Step 1: Write failing behavior tests**

Stub `GraySensorUpdate()` and feed `active_count`, `offset`, and `raw_value`. Verify:

```c
assert(LineFollowInit());
LineFollowTask(false, 0.005f, &output);
assert(!output.line_valid && output.vx_mps == 0.0f);

sample.active_count = 2U;
sample.offset = 0.3f;
LineFollowTask(true, 0.005f, &output);
assert(output.line_valid);
assert(output.vx_mps == LINE_FOLLOW_BASE_SPEED_MPS);
assert(output.wz_radps < 0.0f);
```

Also hold `active_count >= LINE_FOLLOW_A_MARKER_ACTIVE_MIN` for the configured debounce count and verify exactly one `a_marker_event`; release for the rearm count and verify a later marker can trigger again.

- [ ] **Step 2: Run the new test and verify compile failure**

Expected: FAIL because `line_follow.h` does not exist.

- [ ] **Step 3: Implement line sensor ownership and PID**

Register one sensor with the generated `GRAY_SENSOR_GPIO_*` macros. Use `PID_Controller_t` with all tunings and thresholds from the line-follow section of `robot_def.h`. Disabled or invalid samples reset PID and emit zero. Use a small internal debounce state for A-marker edge events.

- [ ] **Step 4: Run line-follow and full host tests**

Expected: the new test passes and all existing host tests remain green.

- [ ] **Step 5: Commit**

```text
feat: 增加H题循迹应用
```

### Task 3: Ball Balance Safety Skeleton

**Files:**
- Create: `app/ball_balance/ball_balance.h`
- Create: `app/ball_balance/ball_balance.c`
- Create: `tests/ball_balance_test.c`
- Create: `tests/ball_balance_stubs/stepper.h`
- Create: `tests/ball_balance_stubs/vision.h`
- Modify: `tests/run_host_tests.ps1`
- Modify: `app/robot_def.h`

- [ ] **Step 1: Write failing safety tests**

Verify initialization selects the configured UART, the task services `Stepper_Task(..., 1U)` every call, disabled commands stop and disable the stepper, and an enabled command without a valid parsed vision frame cannot move the mechanism:

```c
assert(BallBalanceInit());
BallBalanceTask(&command, 1U, 0.001f, &status);
assert(stepper_task_count == 1U);
assert(!status.vision_valid);
assert(stepper_move_count == 0U);
```

- [ ] **Step 2: Run the new test and verify compile failure**

Expected: FAIL because `ball_balance.h` does not exist.

- [ ] **Step 3: Implement the safe boundary**

`BallBalanceInit()` initializes `Vision_Device_t` and `Stepper_Device_t`, leaving EN inactive. `BallBalanceTask()` always services step pulses, publishes position/status, and stops/disables on disabled command or invalid/expired vision. Do not parse bytes or issue movement until the camera frame format is defined.

- [ ] **Step 4: Run ball-balance and full host tests**

Expected: all tests pass; movement count stays zero without a protocol-valid frame.

- [ ] **Step 5: Commit**

```text
feat: 增加滚球平衡安全任务骨架
```

### Task 4: Competition App

**Files:**
- Create: `app/competition/competition.h`
- Create: `app/competition/competition.c`
- Create: `tests/competition_test.c`
- Create: `tests/competition_stubs/key.h`
- Create: `tests/competition_stubs/ti_msp_dl_config.h`
- Modify: `tests/run_host_tests.ps1`
- Modify: `app/robot_def.h`

- [ ] **Step 1: Write failing state tests**

Verify `DISARMED -> READY`, debounced key start, `READY -> RUNNING`, line-loss chassis stop while state remains running, time-limit finish, and init failure fault. Assert all non-running states emit disabled chassis and balance commands.

- [ ] **Step 2: Run the new test and verify compile failure**

Expected: FAIL because `competition.h` does not exist.

- [ ] **Step 3: Implement table-driven handlers**

Use an indexed handler table with one internal handler per state. `CompetitionTask()` reads KEY1, updates debounce and elapsed time, consumes `a_marker_event`, and fills one `Competition_Output_t`. It does not call another app or expose setters/getters.

- [ ] **Step 4: Run competition and full host tests**

Expected: all transition and output assertions pass.

- [ ] **Step 5: Commit**

```text
feat: 增加H题比赛状态任务
```

### Task 5: Robot Scheduling and OLED Integration

**Files:**
- Modify: `app/robot.c`
- Modify: `app/robot_task.h`
- Modify: `app/robot.h`
- Modify: `tests/robot_oled_test.c`
- Modify: `tests/robot_hardware_test_isolation_test.c`
- Create: `tests/robot_schedule_test.c`

- [ ] **Step 1: Update robot tests for app lifecycle and periods**

Stub all four app pairs. Verify normal initialization calls all four `Init` functions once; hardware-test mode calls none; each `RobotTask()` calls ball balance once and calls line/competition/chassis only on every fifth tick. Verify the FreeRTOS robot task period is 1 ms.

- [ ] **Step 2: Run robot tests and verify failure**

Expected: FAIL because `robot.c` still owns old command/chassis logic and uses a 5 ms task period.

- [ ] **Step 3: Implement typed app context and 1 ms scheduling**

Keep a private context in `robot.c` containing line output, competition output, chassis status, and ball status. Initialize all apps before task creation. In `RobotTask()`, call ball every tick and the 5 ms chain in the documented order. Keep IMU/VOFA work on the 5 ms divider. OLED reads only status snapshots.

- [ ] **Step 4: Run robot and full host tests**

Expected: all host tests pass with the new task cadence.

- [ ] **Step 5: Commit**

```text
refactor: 接入H题应用调度链
```

### Task 6: Remove Old Template and Validate Firmware

**Files:**
- Delete: `app/cmd/robot_cmd.c`
- Delete: `app/cmd/robot_cmd.h`
- Delete: `app/cmd/robot_cmd.md`
- Delete: `app/cmd/robot_state_machine.c`
- Delete: `app/cmd/robot_state_machine.h`
- Modify: `app/hardware_test/hardware_test_config.h`
- Modify: `app/application.md`

- [ ] **Step 1: Remove the replaced template and restore normal mode**

Delete `app/cmd`, set `HARDWARE_TEST_MODE` to `HARDWARE_TEST_NONE`, and rewrite the app overview around the four H-topic apps and direct typed task data.

- [ ] **Step 2: Run source checks**

Search for `ROBOT_ENABLE_CMD_APP`, `RobotCMD`, `robot_state_machine`, `chassis_manual_`, and `app/cmd`. Expected: no production references.

- [ ] **Step 3: Run full host tests**

Run: `powershell -ExecutionPolicy Bypass -File tests/run_host_tests.ps1`

Expected: every test prints `PASS`.

- [ ] **Step 4: Generate and build both firmware variants**

Run the configured FreeRTOS and NoRTOS CMake builds. Expected: SysConfig generation succeeds and both ELF files link. Record SysConfig informational warnings separately.

- [ ] **Step 5: Verify Git scope and commit**

Run `git diff --check`, inspect `git status`, and confirm generated build output is not staged.

```text
docs: 更新H题应用层使用说明
```
