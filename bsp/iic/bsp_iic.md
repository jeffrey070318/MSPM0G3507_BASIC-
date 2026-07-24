# IIC

The current project exposes `hi2c1` for `OLED_I2C_INST` at 100 kHz and `hi2c2`
for `MPU_I2C_INST` at 400 kHz. Pin mux, bus clock and electrical settings are
generated from `MSPM0G3507_BASIC.syscfg`.

```c
IIC_Init_Config_s config = {
    .handle = &hi2c1,
    .dev_address = 0x3CU,
    .work_mode = IIC_BLOCK_MODE,
};

IICInstance *device = IICRegister(&config);
```

Device addresses are always 7-bit DriverLib addresses. For example an OLED wire
write address shown as `0x78` in a datasheet must be registered as `0x3C`.

The current TI implementation performs blocking transfers with timeout and
controller error checks. Memory reads use a repeated start. DMA and interrupt
working modes are retained by the YueLu API but are not configured yet.
`IICTransmitEx`, `IICReceiveEx`, and `IICAccessMemEx` return explicit status.
Devices sharing one hardware controller are serialized; a held sequence keeps
ownership until a release transfer or `IICAbortSequence`.
