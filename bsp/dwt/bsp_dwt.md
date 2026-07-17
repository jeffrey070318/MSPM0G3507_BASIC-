# Time Base

The original YueLu DWT API is retained for module compatibility. MSPM0G3507 is
a Cortex-M0+ device, so this implementation does not depend on a DWT cycle
counter. Runtime timelines use the FreeRTOS tick after the scheduler starts,
and `DWT_Delay` uses the TI SDK cycle delay helper.

Before the scheduler starts, timeline functions return zero. Use `DWT_Delay`
for initialization delays while interrupts are disabled.
