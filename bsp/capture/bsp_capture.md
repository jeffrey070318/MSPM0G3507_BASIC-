# Capture BSP

`CAPTURE_0` uses TIMG12 on PB13 with an 80 MHz timer clock. `CAPTURE_1` uses
TIMG8 on PA26 with a 40 MHz timer clock. Both instances are configured for
rising-edge time capture with a 1 ms timer period.

`CaptureRead` returns the raw value latched by the hardware capture channel.
The capture timers count down, so `CaptureReadTimeUs` converts the elapsed
ticks (`period - captured value`) to microseconds by using the timer clock
configured in this project.

If the clock source, divider or SysConfig instance changes, update the clock
constants in `bsp_capture.c` together with the SysConfig configuration.
