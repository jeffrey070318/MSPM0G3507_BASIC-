# Motor 1 DRV8701E Hardware Test Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a one-shot, low-output hardware test for motor 1 that exercises the DRV8701E through the Motor module and exposes encoder feedback for Live Watch.

**Architecture:** A small hardware-independent sequence object owns the timing and fail-safe terminal state. The MSPM0 hardware-test adapter converts that sequence into `Motor_SetOpenLoop()` calls, calls `Motor_Update()` every 10 ms, and publishes encoder telemetry through volatile globals.

**Tech Stack:** C99, TI MSPM0 DriverLib/SysConfig handles, existing Motor/DRV8701E/Encoder modules, FreeRTOS hardware-test task, GCC host tests.

---

### Task 1: Test the one-shot motor sequence

**Files:**
- Create: `tests/hardware_test_motor_sequence_test.c`
- Create: `app/hardware_test/hardware_test_motor_sequence.h`
- Create: `app/hardware_test/hardware_test_motor_sequence.c`
- Modify: `tests/run_host_tests.ps1`

- [x] **Step 1: Write the failing sequence test**

Create a test that initializes the sequence, advances exactly 200/100/200/100 ticks, and checks the expected `0`, `+0.15`, `0`, `-0.15`, and terminal-stop outputs. It must also set an invalid enum value and verify the next step enters the terminal stop state.

```c
#include <assert.h>
#include "hardware_test_motor_sequence.h"

static void StepMany(HardwareTestMotorSequence_t *sequence, unsigned count)
{
    for (unsigned i = 0U; i < count; ++i) {
        HardwareTestMotorSequence_Step(sequence);
    }
}

int main(void)
{
    HardwareTestMotorSequence_t sequence;
    HardwareTestMotorSequence_Init(&sequence);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_WAIT);
    assert(sequence.output == 0.0f);
    assert(!sequence.stop_latched);

    StepMany(&sequence, 199U);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_WAIT);
    HardwareTestMotorSequence_Step(&sequence);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_FORWARD);
    assert(sequence.output == 0.15f);

    StepMany(&sequence, 100U);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_PAUSE);
    assert(sequence.output == 0.0f);
    StepMany(&sequence, 200U);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_REVERSE);
    assert(sequence.output == -0.15f);
    StepMany(&sequence, 100U);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_COMPLETE);
    assert(sequence.output == 0.0f);
    assert(sequence.stop_latched);

    StepMany(&sequence, 100U);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_COMPLETE);
    assert(sequence.stop_latched);

    sequence.stage = (HardwareTestMotorStage_e) 99;
    sequence.stop_latched = false;
    HardwareTestMotorSequence_Step(&sequence);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_COMPLETE);
    assert(sequence.output == 0.0f);
    assert(sequence.stop_latched);

    HardwareTestMotorSequence_Init(NULL);
    HardwareTestMotorSequence_Step(NULL);
    return 0;
}
```

- [x] **Step 2: Run the test and verify RED**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -Iapp/hardware_test tests/hardware_test_motor_sequence_test.c app/hardware_test/hardware_test_motor_sequence.c -o build-host-tests/hardware_test_motor_sequence_test.exe
```

Expected: compilation fails because the sequence header/source and API do not exist.

- [x] **Step 3: Implement the minimal sequence**

Define these public values in `hardware_test_motor_sequence.h`:

```c
typedef enum {
    HARDWARE_TEST_MOTOR_WAIT = 0,
    HARDWARE_TEST_MOTOR_FORWARD,
    HARDWARE_TEST_MOTOR_PAUSE,
    HARDWARE_TEST_MOTOR_REVERSE,
    HARDWARE_TEST_MOTOR_COMPLETE,
} HardwareTestMotorStage_e;

typedef struct {
    HardwareTestMotorStage_e stage;
    uint16_t stage_ticks;
    float output;
    bool stop_latched;
} HardwareTestMotorSequence_t;

void HardwareTestMotorSequence_Init(HardwareTestMotorSequence_t *sequence);
void HardwareTestMotorSequence_Step(HardwareTestMotorSequence_t *sequence);
```

Implement 200 wait ticks, 100 forward ticks, 200 pause ticks, and 100 reverse ticks at a 10 ms caller period. `COMPLETE`, invalid states, and null-safe entry must never produce nonzero output.

- [x] **Step 4: Run the focused test and verify GREEN**

Run the compile command from Step 2 and then:

```powershell
.\build-host-tests\hardware_test_motor_sequence_test.exe
```

Expected: exit code 0.

- [x] **Step 5: Add the focused test to the host runner**

Add an `Invoke-HostTest "hardware_test_motor_sequence_test"` entry using `-Iapp/hardware_test`, the test source, and sequence source. Run `tests/run_host_tests.ps1`; expected: all existing tests plus the new sequence test report `PASS`.

### Task 2: Connect the sequence to Motor 1

**Files:**
- Create: `app/hardware_test/hardware_test_motor.c`
- Modify: `app/hardware_test/hardware_test_config.h`
- Modify: `app/hardware_test/hardware_test_task.c`

- [x] **Step 1: Add and select the motor test mode**

Define `HARDWARE_TEST_MOTOR 7`, set `HARDWARE_TEST_MODE` to it, and extend the validation maximum. Give this mode a 10 ms period in `hardware_test_task.c`.

- [x] **Step 2: Initialize Motor 1 through the DRV8701E backend**

Create a static `Motor_Device_t` and configure:

```c
.type = MOTOR_DRIVER_DRV8701E,
.pwm_handle = &htim1,
.pwm_channel = htim1.Channel,
.pwm_period = 0.00005f,
.phase_port = MOTOR_GPIO_AIN1_PORT,
.phase_pin = MOTOR_GPIO_AIN1_PIN,
.encoder = &hencoder_left,
.encoder_reverse = false,
```

Use a valid but inactive speed PID configuration with output limit `1.0f`. Initialize the sequence before `Motor_Init()`, expose whether `motor.initialized` is true, and return `DEVICE_ERROR` without applying output if initialization fails.

- [x] **Step 3: Apply sequence output and publish telemetry**

Each 10 ms call must:

1. advance `HardwareTestMotorSequence_Step()`;
2. call `Motor_SetOpenLoop()` with the sequence output while not terminal;
3. call `Motor_Stop()` once terminal and the motor is still enabled;
4. call `Motor_Update(&motor, 0.01f)` to snapshot the encoder;
5. copy stage, output, total count, delta count, counts/s, and invalid-transition count into volatile globals.

Use `Encoder_Get_Total()`, `Encoder_Get_Speed()`, and `Encoder_Get_InvalidTransitions()` rather than reading encoder fields directly.

- [x] **Step 4: Build all firmware variants**

Run:

```powershell
cmake --build build --clean-first --target all
cmake --build build-link-check --clean-first --target all
cmake --build build-nortos --clean-first --target all
```

Expected: all three exit with code 0. The FreeRTOS and link-check builds compile the sequence and `hardware_test_motor.c`; NoRTOS validates the shared Motor implementation.

### Task 3: Document and package the hardware test

**Files:**
- Modify: `框架使用说明.md`
- Add/move/delete: the current user-provided PNG/PDF documentation changes already present in the worktree

- [x] **Step 1: Update the hardware-test instructions**

Add `MOTOR` to the mode list and state that motor 1 runs the one-shot 2 s wait, 15% forward, 2 s stop, 15% reverse, permanent-stop sequence. List the Live Watch variables and require the wheel to be off the ground.

- [x] **Step 2: Run final checks**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_host_tests.ps1
python C:\Users\LENOVO\.codex\skills\mspm0-skill-main\mspm0-skill-main\skills\mspm0-ccs\scripts\check_syscfg.py .
git diff --check
```

Expected: host tests and SysConfig check exit 0; `git diff --check` reports no whitespace errors. Report SysConfig informational messages separately from build success.

- [x] **Step 3: Commit implementation and reference assets**

Stage the motor test source, tests, documentation, and all current user-provided image/PDF additions, moves, and deletions. Commit with:

```powershell
git commit -m "test: add motor1 DRV8701E hardware check"
```
