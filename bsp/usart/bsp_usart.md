# USART

The current board configuration exposes `huart1`, `huart2`, and `huart3`,
mapped to the generated `UART1_INST`, `UART2_INST`, and `UART3_INST` macros.
SysConfig owns their pins, clocks, and baud rates.

```c
USART_Init_Config_s config = {
    .recv_buff_size = 64U,
    .usart_handle = &huart1,
    .module_callback = ProtocolDecode,
};

USARTInstance *uart = USARTRegister(&config);
```

Blocking transmission is available on all three handles. Only `huart1` has
SysConfig RX/TX DMA and UART interrupts, so interrupt/DMA transmission and the
automatic receive callback are limited to `huart1`. `UART0_IRQHandler` is
maintained in `mspm0_irq.c`, outside generated SysConfig files.

UART1 RX uses DMA plus the SysConfig RX timeout interrupt, so a frame is
dispatched either when the configured buffer fills or when a shorter frame goes
idle. DMA/interrupt TX completion waits for both data loading and EOT and is not
dependent on interrupt priority order. Blocking TX has a bounded polling timeout.
