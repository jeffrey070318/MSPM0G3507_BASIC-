# H 题应用层

`app/` 只负责整车策略和控制组合。通用设备能力在 `module/`，寄存器、引脚和中断在 `bsp/` 与 `MSPM0G3507_BASIC.syscfg`。

## 目录职责

| 目录 | 职责 | 独占资源 |
| --- | --- | --- |
| `competition/` | 按键模式选择、比赛状态、计时、安全命令 | KEY1、KEY2 |
| `line_follow/` | 八路灰度、线路偏差、A 标志、循迹 PID | 灰度模块 |
| `chassis/` | 差速运动学、左右轮编码器速度环 | 两路 DRV8701E 电机 |
| `ball_balance/` | 视觉有效性、平衡外环边界、步进服务 | 视觉串口、STEP/DIR/EN |
| `hardware_test/` | 单项硬件测试入口 | 由测试模式决定 |

每个 app 的头文件只公开 `Init()` 和 `Task()`。细分操作保留为 `.c` 内部静态函数，不暴露 BSP 句柄或 module 设备结构体。

## 调度

正常模式只有一个 1 ms Robot 控制任务和一个 200 ms OLED 任务：

1. `BallBalanceTask()` 每 1 ms 运行，保证 `Stepper_Task()` 的服务周期。
2. 每 5 ms 依次运行 `LineFollowTask()`、`CompetitionTask()`、`ChassisTask()`。
3. `competition` 是唯一整车决策者；`robot.c` 只保存并传递有类型的命令和状态快照。
4. OLED 只读取状态快照，不参与控制。

应用之间当前是一对一固定数据流，不使用消息中心。增加新消费者前不要预先引入发布订阅层。

## 当前边界

- 循迹已具备低速初始 PID、失线停机和 A 标志单次事件，参数仍需实车调试。
- 底盘已接入两路 DRV8701E 和编码器速度闭环。
- KEY1 选择循迹模式，KEY2 选择滚球平衡/水管模式。
- 平衡任务已安全初始化视觉与步进并提供 1 ms 服务；`module/pipe_axis` 已提供水管机构解算，但相机协议和控制策略未确定，因此暂不发运动命令。
- 默认 `HARDWARE_TEST_MODE` 为 `HARDWARE_TEST_NONE`。做单项测试时临时切换，结束后恢复正常模式。

调参统一修改 `app/robot_def.h`。引脚和外设配置只修改根目录 `.syscfg`，不要手改 `build/syscfg` 生成文件。
