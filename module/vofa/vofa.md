# VOFA

The module owns the fixed `huart1` BSP registration and emits VOFA+ JustFloat
frames. Application code initializes and sends data without registering USART
or passing UART handles.

```c
VOFA_Init();

const float data[] = {1.0f, 2.0f, 3.0f};
VOFA_JustFloatOutputDMA(data, 3U);
```

`VOFA_JustFloatOutput()` uses blocking transmission and
`VOFA_JustFloatOutputDMA()` uses DMA. Both append the JustFloat frame tail
`00 00 80 7F` and return `Device_Status_e`.
