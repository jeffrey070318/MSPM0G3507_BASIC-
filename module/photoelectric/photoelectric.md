# Photoelectric Switch Module

> 注意：本模块目前仅完成形式封装，尚未经过认真硬件测试和针对实际设备的修正。

This module reads one digital photoelectric switch.

```c
Photoelectric_Device_t sensor;

Photoelectric_Init(
    &sensor, GPIO_PORT, GPIO_PIN, GPIO_PIN_RESET);
bool blocked = Photoelectric_IsTriggered(&sensor);
```

`active_state` is the input level reported as triggered. Filtering, edge
callbacks, and event counting are intentionally left out of this first wrapper.
