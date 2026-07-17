# USART

The current board configuration exposes `UART_0_INST` at 115200 baud. SysConfig
owns the UART pins, clocks, interrupt sources and RX/TX DMA channels.

```c
USART_Init_Config_s config = {
    .recv_buff_size = 64U,
    .usart_handle = &huart1,
    .module_callback = ProtocolDecode,
};

USARTInstance *uart = USARTRegister(&config);
```

`USARTSend` supports blocking, interrupt and DMA transmission. Reception uses
the configured DMA channel; a completed receive calls the registered module
callback and then restarts DMA. `UART0_IRQHandler` is maintained in
`mspm0_irq.c`, outside generated SysConfig files.
