# GPIO

GPIO pins and interrupt edges are owned by SysConfig. A module registers a
`GPIOInstance` with a TI port pointer and pin mask.

```c
GPIO_Init_Config_s config = {
    .GPIOx = GPIOA,
    .GPIO_Pin = GPIO_PIN_6,
    .pin_state = GPIO_PIN_RESET,
    .exti_mode = GPIO_EXTI_MODE_FALLING,
};

GPIOInstance *gpio = GPIORegister(&config);
GPIOSet(gpio);
GPIOReset(gpio);
```

Registration accepts exactly one pin bit and rejects duplicate port/pin pairs.
Registered pins remain owned until the MCU resets. GPIO registration is
protected against the maintained GPIO ISR.
`exti_mode` records module intent only; the real interrupt edge remains owned by
SysConfig.

The four high-rate encoder inputs are handled directly by the encoder BSP in
the maintained Group 1 IRQ entry. For future GPIO interrupts, the port-aware
`GPIOInterruptCallbackForPort` and legacy pin-only `GPIOInterruptCallback`
dispatchers remain available. The Group 1 IRQ routes non-encoder GPIO events to
the port-aware dispatcher.
