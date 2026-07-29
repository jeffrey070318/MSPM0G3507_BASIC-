# Stepper

This module drives one external STEP/DIR stepper driver from MSPM0 GPIO.

Default wiring:

| Signal | MCU pin | Note |
| --- | --- | --- |
| STEP | PB0 | Rising edge step pulse |
| DIR | PB5 | High is up, low is down |
| EN | PB6 | Active low, default disabled |

Use an external stepper driver and motor power supply. Do not connect a 42 mm
stepper motor directly to the MSPM0 pins. Connect MCU GND and driver signal GND
together, and confirm that the driver's logic input accepts 3.3 V.

For the pipe-balancing axis, keep the motor driver in internal closed-loop FOC
and external pulse mode. The robot controller should run the outer loop from
ball-position error and issue small relative moves through `Stepper_Move()`.

Recommended starting settings:

| Item | Value |
| --- | --- |
| Driver mode | `CR_VFOC` |
| Pulse input | `PUL_ENA` |
| Serial input | `RxTx_OFF` |
| Enable | `Hold` during tuning |
| Microstep | 16 first, 32 if the mechanism needs finer motion |

Use a deadband around the target ball position, limit the maximum step command
per control cycle, and keep the stepper speed low enough that the water-pipe
mechanism does not overshoot from inertia.
