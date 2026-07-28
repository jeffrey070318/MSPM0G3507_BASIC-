# Chassis

当前实现是两轮差速底盘，不是麦克纳姆底盘。`ChassisInit()` 创建左右两个 DRV8701E 闭环电机，`ChassisTask()` 接收线速度 `vx` 和角速度 `wz`，完成差速解算并周期调用 `Motor_Update()`。

[车架结构参考图](./车架图.png)

## 方向配置

`CHASSIS_LEFT_MOTOR_REVERSE` 和 `CHASSIS_RIGHT_MOTOR_REVERSE` 是左右轮各自唯一的反向配置。初始化时同一个值会同时写入电机驱动方向和编码器反馈方向，避免 PID 因反馈符号相反形成正反馈并持续加大输出。

首次接线仍需架空车轮确认闭环符号：给出较小正目标时，测量速度应为正，且误差随加速减小。若符号错误，应先检查电机线序和编码器 A/B 相；确认基础闭环正确后，才用上述宏整体翻转某一侧的车体前进方向。

## 调参位置

底盘实车参数统一放在 `app/robot_def.h`，`chassis.c` 不再保存参数副本：

| 参数 | 用途 |
| --- | --- |
| `CHASSIS_WHEEL_RADIUS_M` | 车轮半径，单位 m |
| `CHASSIS_TRACK_WIDTH_M` | 左右轮中心距，单位 m |
| `CHASSIS_ENCODER_PPR` | 编码器单通道每圈脉冲数 |
| `CHASSIS_ENCODER_QUADRATURE` | AB 相解码倍率，当前为 4 |
| `CHASSIS_MOTOR_GEAR_RATIO` | 电机到车轮的减速比 |
| `CHASSIS_SPEED_KP/KI/KD` | 两路速度 PID 参数 |
| `CHASSIS_SPEED_MAX_OUT/MAX_IOUT` | PID 输出和积分限幅 |
| `CHASSIS_LEFT/RIGHT_MOTOR_REVERSE` | 左右轮驱动与反馈同步反向 |

INS 的编码器里程换算直接引用同一组轮径、PPR、倍率和减速比，因此只需修改一处。

## 运行流程

1. 接收速度指令；没有有效指令时进入 `CHASSIS_ZERO_FORCE`。
2. 按轮距计算左右轮目标线速度。
3. 根据轮径、编码器线数和减速比换算为 `counts/s`。
4. 更新左右编码器速度 PID 并输出到 DRV8701E。
5. 启用 INS 时，通过消息中心发布左右累计编码器值。

## 正式联调入口

当前已启用 `ROBOT_ENABLE_CHASSIS_APP`，`ChassisInit()` 会直接注册两路 DRV8701E：左轮使用 `htim1 + MOTOR_GPIO_AIN1 + hencoder_left`，右轮使用 `htim2 + MOTOR_GPIO_BIN1 + hencoder_right`。上电默认保持 `CHASSIS_ZERO_FORCE`，不会自动前进。

未启用 INS 时，可调用：

```c
ChassisSetManualCommand(0.05f, 0.0f);
ChassisDisableManualCommand();
```

也可以在 Live Watch/Global Variables 中先设置 `chassis_manual_vx_mps` 和 `chassis_manual_wz_radps`，最后将 `chassis_manual_enabled` 改为 `true`。停止时先将 `chassis_manual_enabled` 改为 `false`。速度单位分别为 `m/s` 和 `rad/s`。

正式电机对象为 `chassis_motors[0]`（左轮）和 `chassis_motors[1]`（右轮），可直接观察 `target_speed`、`measured_speed`、`control_output` 和 `speed_pid`。首次测试必须架空车轮，建议从 `vx=0.05 m/s, wz=0` 开始，并准备断电急停。

## 当前状态

电机 1、左编码器和 DRV8701E 已完成单轮速度闭环实测。两路正式实例已接入并通过宿主注册测试；右轮方向、整车轮距、轮径、编码器参数、转向符号和双轮运行仍需本轮实测确认。

INS 返航模板默认关闭；只有同时定义 `ROBOT_ENABLE_CHASSIS_APP` 和 `ROBOT_ENABLE_INS_APP` 时才接入消息中心。平衡底盘、麦克纳姆底盘和舵轮只保留扩展入口，尚无可用实现。
