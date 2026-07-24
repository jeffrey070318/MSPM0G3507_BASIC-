# Electromagnet Module

> 注意：本模块目前仅完成形式封装，尚未经过认真硬件测试和针对实际设备的修正。

This module controls a relay-driven electromagnet through one GPIO output.

```c
Electromagnet_Device_t magnet;

Electromagnet_Init(
    &magnet, GPIO_PORT, GPIO_PIN, GPIO_PIN_SET);
Electromagnet_On(&magnet);
Electromagnet_Off(&magnet);
```

`active_state` is the relay input level that energizes the electromagnet.
Initialization always drives the opposite level so the relay starts inactive.
Pulse timing and automatic release remain application responsibilities.
