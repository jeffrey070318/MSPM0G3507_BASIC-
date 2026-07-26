# Motor 模块

Motor 将功率级、编码器和速度 PID 组合为一个由调用者持有的闭环电机对象。模块不分配 `Motor_Device_t`，不维护全局电机数组，也不负责调度。

## 所有权

- app 静态创建 `Motor_Device_t`；
- `Motor_Init()` 注册该电机所需的 PWM/GPIO 资源；
- 编码器实例由 BSP 提供，Motor 只绑定、启动和读取；
- Motor 及其 PWM/GPIO 资源持有到 MCU 复位，运行时使用 `Motor_Stop()` 停止输出。

## 速度单位

`Motor_SetTargetSpeed()` 和 `measured_speed` 均使用 `encoder counts/s`。`Motor_Update()` 读取本周期编码器增量，并使用调用者传入的真实 `dt_seconds` 换算速度。

## 方向约定

速度闭环中，正输出必须产生正的编码器速度，否则 PID 会把负反馈变成正反馈并持续加大输出。底盘层使用每路电机唯一的 `REVERSE` 配置，并同时赋给驱动的 `reverse` 和 Motor 的 `encoder_reverse`。不要只修改其中一个。

首次接线需要用较小输出架空测试，确认正目标、正转向和正反馈三者一致。驱动线序与编码器 A/B 相的基础符号若不匹配，应先修正接线或编码器相序；基础闭环正确后，整体翻转两个反向配置只用于改变该轮在车体坐标系中的正方向。

## DRV8701E

DRV8701E 使用 `EN/PWM + PH/DIR`：

- 输出正负号决定 PH；
- 输出绝对值决定 EN 的 PWM 占空比；
- 换向时先将 EN 占空比归零，再改变 PH；
- EN=0 是低侧慢衰减制动，不是高阻滑行；
- 当前双路板的 nSLEEP 由板载开关控制，软件不能请求 coast。

当前扩展板映射：

| 电机 | EN/PWM | PH/DIR | 未使用的 TB6612 方向脚 |
| --- | --- | --- | --- |
| 左 | PA0 / `htim1` | PA17 / AIN1 | PA16 / AIN2 |
| 右 | PA1 / `htim2` | PB4 / BIN1 | PB1 / BIN2 |

接线资料：

- [新电机线序](./新电机线序.png)
- [老车旧电机接口线序](./老车老电机接口线序.png)

## TB6612

TB6612 使用 `PWM + IN1 + IN2`。初始化配置可以选择 `MOTOR_STOP_COAST` 或 `MOTOR_STOP_BRAKE`，其余速度闭环逻辑与 DRV8701E 共用。

## 调用方式

```c
static Motor_Device_t left_motor;

Motor_Init_Config_t config = {
    .driver = {
        .type = MOTOR_DRIVER_DRV8701E,
        .config.drv8701e = {
            .pwm_handle = &htim1,
            .pwm_channel = htim1.Channel,
            .pwm_period = 0.00005f,
            .phase_port = GPIOA,
            .phase_pin = GPIO_PIN_17,
            .reverse = false,
        },
    },
    .encoder = &hencoder_left,
    .encoder_reverse = false,
    .speed_pid = {
        .kp = 0.000025f,
        .ki = 0.0002f,
        .kd = 0.0f,
        .output_limit = 1.0f,
        .integral_limit = 0.5f,
        .derivative_on_measurement = true,
    },
};

Motor_Init(&left_motor, &config);
Motor_SetTargetSpeed(&left_motor, target_counts_per_second);
Motor_Update(&left_motor, dt_seconds);
```

硬件测试可调用 `Motor_SetOpenLoop()` 直接设置 `-1.0f` 到 `1.0f` 的归一化输出。调用 `Motor_SetTargetSpeed()` 会重新切回速度闭环模式。

电机 1、左编码器和 DRV8701E 已完成 `1500 counts/s` 单轮速度闭环实测。右轮方向和双轮底盘仍需实测。
