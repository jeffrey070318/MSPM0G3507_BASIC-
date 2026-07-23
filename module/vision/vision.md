# Vision Communication Module

> 注意：本模块目前仅完成形式封装，尚未经过认真硬件测试和针对实际设备的修正。

This module gives camera communication a named API while leaving the wire
protocol undefined. It currently forwards raw bytes through `transparent_uart`.

```c
Vision_Device_t vision = {0};
uint8_t command[] = {0x01U};
uint8_t response[32];
uint16_t received;

Vision_Init(&vision, TRANSPARENT_UART_PORT_2);
Vision_Send(&vision, command, sizeof(command));
Vision_Read(&vision, response, sizeof(response), &received);
```

Frame headers, lengths, checksums, commands, and parsing must be added only
after the actual camera protocol is selected.
