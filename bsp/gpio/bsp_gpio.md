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

When a GPIO interrupt is configured, the maintained IRQ entry passes the
pending pin mask to `GPIOInterruptCallback`.
