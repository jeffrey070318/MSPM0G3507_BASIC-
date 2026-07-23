# PWM

PWM timers, channels, pins and initial periods are generated from
`MSPM0G3507_BASIC.syscfg`. A module registers the generated timer instance and a
`PWM_CHANNEL_x` channel.

```c
PWM_Init_Config_s config = {
    .htim = &htim1,
    .channel = TIM_CHANNEL_1,
    .period = 0.00005f,
    .dutyratio = 0.0f,
};

PWMInstance *pwm = PWMRegister(&config);
```

Changing a timer period affects every PWM channel on that timer.
The BSP synchronizes the stored period and recalculates every registered
channel on the same timer. Stopping one channel drives only that output inactive;
the physical timer stops only after its final registered channel is stopped.
The descriptor also records whether the timer counts up or down so the requested
duty ratio is converted to the correct capture/compare value.
`PWMStartDMA` currently only starts the PWM because SysConfig does not own a
PWM DMA channel yet.
