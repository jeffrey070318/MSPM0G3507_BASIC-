# Algorithm 模块

`module/algorithm` 放置不依赖 DriverLib、FreeRTOS 或具体外设的算法实现。

## PID 控制器

头文件：`algorithm/pid.h`

当前 PID 使用调用者显式传入的 `dt_seconds`，提供：

- P、I、D；
- 对称输出限幅；
- 对称积分限幅；
- 条件积分抗饱和；
- 可选死区；
- 可选微分先行；
- 状态清零和运行时修改 `Kp/Ki/Kd`。

PID 不读取 DWT，也不包含角度环绕、位置环、滤波或堵转判断。调用周期和故障策略由使用该算法的上层模块决定。

```c
PID_Controller_t pid;
PID_Config_t config = {
    .kp = 0.001f,
    .ki = 0.0002f,
    .kd = 0.0f,
    .output_limit = 1.0f,
    .integral_limit = 0.5f,
    .derivative_on_measurement = true,
};

PID_ControllerInit(&pid, &config);
float output = PID_ControllerUpdate(&pid, target, feedback, dt_seconds);
```

工程中的 PID 统一使用 `module/algorithm/pid.h`，业务模块应显式包含 `algorithm/pid.h`。
