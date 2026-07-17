# IIC

The current project exposes `I2C_0_INST`. Pin mux, bus clock and electrical
settings are generated from `MSPM0G3507_BASIC.syscfg`.

```c
IIC_Init_Config_s config = {
    .handle = &hi2c1,
    .dev_address = 0x3CU,
    .work_mode = IIC_BLOCK_MODE,
};

IICInstance *device = IICRegister(&config);
```

The current TI implementation performs blocking transfers with timeout and
controller error checks. Memory reads use a repeated start. DMA and interrupt
working modes are retained by the YueLu API but are not configured yet.
