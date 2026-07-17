# ADC

ADC channel, reference, resolution and sample time are configured in
`empty.syscfg`. The maintained BSP starts one conversion and reads the
generated ADC memory slot.

```c
uint16_t raw_value;
if (ADCRead(&raw_value, 0U) == DEVICE_OK) {
    /* Use raw_value. */
}
```

A zero timeout selects `ADC_DEFAULT_TIMEOUT`. The timeout is a polling count,
not a millisecond value.
