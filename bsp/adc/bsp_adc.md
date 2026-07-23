# ADC

The current SysConfig does not own an ADC instance, so `ADCRead` returns
`DEVICE_ERROR`. After an ADC12 instance and memory slot are added to
`MSPM0G3507_BASIC.syscfg`, update the BSP to use the generated instance and
memory macros instead of assuming a fixed name.

```c
uint16_t raw_value;
if (ADCRead(&raw_value, 0U) == DEVICE_OK) {
    /* Use raw_value. */
}
```

A zero timeout selects `ADC_DEFAULT_TIMEOUT`. The timeout is a polling count,
not a millisecond value.
