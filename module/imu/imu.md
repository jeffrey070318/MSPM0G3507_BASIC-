# IMU

JY901S 10-axis IMU over I2C, connected via `hi2c2` / `MPU_I2C`. The module reads
acceleration, angular velocity, Euler angles and magnetometer data from
on-chip registers 0x34–0x37 through the IIC driver.

```c
IMU_Init();

IMU_Data_t data;
if (IMU_ReadAll(&data) == DEVICE_OK) {
    /* data.roll, data.pitch, data.yaw  -- deg */
    /* data.gx,   data.gy,   data.gz    -- deg/s */
    /* data.ax,   data.ay,   data.az    -- m/s^2 */
}
```

The module owns the fixed `hi2c2` BSP registration internally. Application
code does not construct IIC configuration objects or pass BSP handles.

Scale factors assume the JY901S default full-scale ranges: ±16 g accelerometer,
±2000 °/s gyroscope, ±180° angles. Raw int16 values are available through
`IMU_ReadRaw` for custom scaling. DMA and interrupt modes are not configured;
all reads are blocking.
