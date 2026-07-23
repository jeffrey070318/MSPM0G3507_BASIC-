# Capture BSP

The current SysConfig does not own a capture timer. The capture BSP therefore
returns zero and performs no hardware operation until `CAPTURE_0` or
`CAPTURE_1` is added back to `MSPM0G3507_BASIC.syscfg`.

`CaptureRead` returns the raw value latched by the hardware capture channel.
The capture timers count down, so `CaptureReadTimeUs` converts the elapsed
ticks (`period - captured value`) to microseconds by using the timer clock
configured in this project.

After adding a capture instance, verify its generated instance, load value,
clock, channel, and IRQ macros before updating `bsp_capture.c`.
