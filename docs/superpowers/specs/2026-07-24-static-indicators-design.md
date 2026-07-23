# Static LED And Buzzer Design

## Lifecycle

GPIO and PWM resources are registered once during startup and remain owned until
the MCU resets. Initialization failure is terminal for that startup attempt;
modules report failure but do not unregister resources or retry in place.

The public `GPIOUnregister`, `PWMUnregister`, `Motor_Deinit`, and motor-driver
deinitialization chain are removed. Duplicate registration checks remain so a
configuration mistake is still detected.

## LED

`module/led` provides caller-owned LED objects. `LED_Init()` binds one object to
one active-high GPIO, and every on, off, or toggle call explicitly receives the
target LED object. Board-level code chooses whether an object represents LED3,
LED4, the board LED, or another output.

## Buzzer

`module/buzzer` owns the active-high `BUZZER` GPIO output. It provides one-time
initialization plus on, off, and toggle operations. It has no PWM, melody
sequencer, FreeRTOS task, or periodic update function.

## Key

`module/key` provides caller-owned key objects. `KEY_Init()` binds one object to
one GPIO and records whether high or low is the pressed state. `KEY_IsPressed()`
returns the current physical state without debounce, edge detection, long-press
tracking, interrupts, or a periodic task.

## Servo

`module/servo` provides caller-owned servo objects. `SERVO_Init()` binds one
object to one BSP PWM handle and caller-supplied minimum/maximum pulse widths.
The PWM period is 20 ms. `SERVO_SetAngle()` clamps 0--180 degrees and linearly
maps it to pulse width; `SERVO_Stop()` disables the PWM output. The module has
no motion profile, feedback, task, or multi-servo scheduler.

## Failure Behavior

Operations before successful initialization and null LED objects are no-ops.
Initialization returns `false` when resource registration fails so the
startup owner can stop or report the boot failure.

## Verification

Host tests cover active-high output behavior, toggle behavior, active-high and
active-low keys, servo angle/pulse conversion, null objects, and initialization
failure. Existing GPIO, PWM, motor, and full firmware builds verify removal of
the deinitialization APIs.
