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

Blocking transmission is available on all three handles. SysConfig now assigns
RX/TX DMA channels and UART interrupts to all three UARTs, but the maintained
BSP async implementation still dispatches only `huart1` through
`UART0_IRQHandler`. UART2/3 DMA and receive callbacks require a follow-up BSP
adaptation before modules can use them.

`USARTReceiveAvailable()` is available on all three handles and drains only the
bytes currently in the hardware RX FIFO. It is suitable for simple polling
transparent links but does not preserve bytes if the application polls too
slowly. UART1 asynchronous RX uses DMA; the shorter-frame timeout path only
runs when the SysConfig RX timeout interrupt is enabled. DMA/interrupt TX
completion waits for both data loading and EOT and is not dependent on interrupt
priority order. Blocking TX has a bounded polling timeout.
