# Encoder BSP

The current board uses two A/B quadrature encoders through four GPIO
interrupt inputs:

- left: PA12 / PA13
- right: PB20 / PB23

All four pins trigger on rising and falling edges, so one complete A/B cycle
produces four counts. `GROUP1_IRQHandler` reads both phase levels and feeds a
four-state transition table. The state sequence `00 -> 01 -> 11 -> 10 -> 00`
is positive; call `Encoder_SetReverse()` when the installed wheel needs the
opposite sign.

`hencoder_left` and `hencoder_right` are initialized by `BSPInit()`. The total
count is maintained directly in the GPIO ISR. Call `Encoder_Update()` at a
fixed application period to snapshot the count delta into `speed`; therefore
speed is counts per update period, not RPM. Convert it in the chassis layer
using the update period, encoder line count, gear ratio, and wheel diameter.

Skipped two-bit transitions are not assigned a direction. They increment
`invalid_transition_count`, which is useful for detecting excessive speed,
contact bounce, weak pull-ups, or signal integrity problems.

Set `HARDWARE_TEST_MODE` to `HARDWARE_TEST_ENCODER` and observe:

- `hardware_test_encoder_left_total`
- `hardware_test_encoder_right_total`
- `hardware_test_encoder_left_speed`
- `hardware_test_encoder_right_speed`
- `hardware_test_encoder_left_invalid`
- `hardware_test_encoder_right_invalid`

The test updates speed every 10 ms.
