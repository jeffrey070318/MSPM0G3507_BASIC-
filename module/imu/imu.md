# IMU

JY901S 10-axis IMU over I2C, connected via `I2C_0_INST`. The BSP reads
acceleration, angular velocity, Euler angles and magnetometer data from
on-chip registers 0x34–0x37 through the IIC driver.

```c
IIC_Init_Config_s imu_conf = {
    .handle      = &hi2c1,
    .dev_address = JY901S_I2C_ADDR,  /* 0x50 */
    .work_mode   = IIC_BLOCK_MODE,
};

IMU_Handle_t imu;
IMU_Init(&imu, &imu_conf);

IMU_Data_t data;
if (IMU_ReadAll(&imu, &data) == DEVICE_OK) {
    /* data.roll, data.pitch, data.yaw  -- deg */
    /* data.gx,   data.gy,   data.gz    -- deg/s */
    /* data.ax,   data.ay,   data.az    -- m/s^2 */
}
```

Scale factors assume the JY901S default full-scale ranges: ±16 g accelerometer,
±2000 °/s gyroscope, ±180° angles. Raw int16 values are available through
`IMU_ReadRaw` for custom scaling. DMA and interrupt modes are not configured;
all reads are blocking.