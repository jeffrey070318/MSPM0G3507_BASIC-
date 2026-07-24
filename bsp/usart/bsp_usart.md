# USART

> 注意：UART2/3 的 DMA+超时接收已完成源码、SysConfig 和构建验证，尚未完成
> 连接实际外设后的连续二进制流压力测试。

The current board configuration exposes `huart1`, `huart2`, and `huart3`,
mapped to the generated `UART1_INST`, `UART2_INST`, and `UART3_INST` macros.
SysConfig owns their pins, clocks, and baud rates.

```c
USART_Init_Config_s config = {
    .recv_buff_size = 64U,
    .usart_handle = &huart1,
    .module_callback = NULL,
};

USARTInstance *uart = USARTRegister(&config);
```

Blocking transmission is available on all three handles. Each configured UART
has an independent RX DMA channel, IRQ entry, and software ring buffer. The BSP
uses a short DMA transaction and handles both `DMA_DONE_RX` and
`RX_TIMEOUT_ERROR`; it then drains any residual FIFO bytes before rearming DMA.
This preserves a byte stream even when the application task does not poll on
every character.

`USARTReceiveAvailable()` is non-blocking and returns the bytes currently in the
software ring buffer. It does not imply a complete frame; framing, length, and
checksum rules belong to the consuming module. The ring buffer drops newest
bytes only after becoming full and accumulates the count in
`USARTInstance.rx_drop_count`. UART error events are counted in
`rx_error_count`/`rx_overrun_count`, the FIFO is drained, and the corresponding
interrupt flags are cleared before DMA is rearmed. UART2/3 asynchronous TX is
intentionally not enabled yet; their sends use bounded blocking transmission.
DMA/interrupt TX on UART1 waits for both data loading and EOT.

`recv_buff_size` controls one DMA segment and may be smaller than the 256-byte
software ring. Set the ring consumer's polling/processing rate so the ring does
not remain full during sustained input. The polling ring is the
`module_callback == NULL` path; callback registrations retain their queued
callback behavior instead.
