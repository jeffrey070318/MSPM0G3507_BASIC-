# Transparent UART Module

> 注意：本模块目前仅完成形式封装，尚未经过认真硬件测试和针对实际设备的修正。

This module provides raw byte transmission for devices without a fixed
application protocol. UART2 and UART3 are selectable at initialization, so
either port can later be assigned to a camera or a wireless link.

```c
TransparentUART_Device_t link = {0};
uint8_t command[] = {0x01U, 0x02U};
uint8_t response[32];
uint16_t received;

TransparentUART_Init(&link, TRANSPARENT_UART_PORT_2);
TransparentUART_Send(&link, command, sizeof(command));
TransparentUART_Read(&link, response, sizeof(response), &received);
```

Sending is blocking. Reading drains only bytes currently present in the UART
receive FIFO and may return zero bytes with `DEVICE_OK`. The module does not
parse frames or assign camera and wireless roles to specific ports.
