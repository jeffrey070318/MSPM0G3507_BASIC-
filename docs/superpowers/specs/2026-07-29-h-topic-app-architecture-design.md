# H 题 App 架构设计

## 1. 目标与范围

本设计把当前通用模板式 `app/` 收敛为 H 题车辆的应用层：车辆使用八路灰度传感器循迹，以两路 DRV8701E 电机完成差速运动，以视觉坐标作为滚球位置反馈，并用 STEP/DIR/EN 步进执行机构调节平衡杆。

本阶段只确定职责、接口、时序、安全策略和迁移路径。视觉字节协议、PID 最终数值、A 标志的实车判定阈值、步进机构行程和方向必须由后续单项测试确定，不在架构层预设。

## 2. 设计原则

- `competition` 是唯一的整车决策者，负责模式、阶段、计时、启停和故障降级。
- `line_follow` 只处理灰度数据并产生底盘运动建议，不直接操作电机。
- `chassis` 独占两路底盘电机，只接受带使能位的速度命令。
- `ball_balance` 独占步进设备，只接受目标位置和使能命令。
- `robot.c` 只负责初始化、固定周期调度和状态汇总，不保存比赛状态，不实现 PID。
- 当前一对一数据流使用有类型的直接函数调用，不为未来可能出现的消费者预先引入消息中心。
- BSP 和 module 层保持通用；赛题流程、单位换算、限幅和参数归 app 层。

## 3. 目录与所有权

```text
app/
├─ competition/
│  ├─ competition.c
│  └─ competition.h
├─ line_follow/
│  ├─ line_follow.c
│  └─ line_follow.h
├─ ball_balance/
│  ├─ ball_balance.c
│  └─ ball_balance.h
├─ chassis/
│  ├─ chassis.c
│  ├─ chassis.h
│  └─ chassis.md
├─ hardware_test/
├─ robot.c
├─ robot.h
├─ robot_task.h
└─ robot_def.h
```

### `competition`

负责按键启动、比赛模式、比赛阶段、计时、A 标志事件消费、整车使能和安全停机。它读取 `line_follow` 与 `ball_balance` 的状态，向二者下发是否运行及目标，不访问 BSP 引脚，也不直接修改 Motor 或 Stepper 结构体。

### `line_follow`

注册并更新八路灰度传感器，计算线路偏差、线路有效性和 A 标志事件，运行循迹 PID 与速度规划，输出 `vx/wz` 建议。A 标志使用单次事件语义：一次连续识别只产生一次事件，离开标志区域并完成去抖后才允许再次触发。

### `chassis`

注册左右两个 DRV8701E 电机和编码器，完成差速运动学和左右轮编码器速度闭环。该目录不再读取全局“手动变量”，正式接口只接收完整命令；调试命令也必须通过同一接口下发。

### `ball_balance`

注册视觉设备和 `Stepper_Device_t`，解析完整视觉帧，维护最新球位置与时间戳，运行球位置外环，并把输出转换为有界的相对步数命令。它不决定比赛何时开始，也不控制底盘。

## 4. 公共接口

公共头文件只暴露命令、状态和生命周期函数，不暴露 BSP 句柄或内部设备实例。

### 底盘

```c
typedef struct {
    float vx_mps;
    float wz_radps;
    bool enabled;
} Chassis_Command_t;

typedef struct {
    float left_target_counts_s;
    float left_measured_counts_s;
    float right_target_counts_s;
    float right_measured_counts_s;
    bool enabled;
} Chassis_Status_t;

bool Chassis_Init(void);
void Chassis_SetCommand(const Chassis_Command_t *command);
void Chassis_Update(float dt_seconds);
void Chassis_Stop(void);
void Chassis_GetStatus(Chassis_Status_t *status);
```

`Chassis_SetCommand()` 复制输入，不保存调用者指针。`enabled == false`、线路丢失或比赛停止最终都必须走 `Chassis_Stop()`，将目标清零并关闭两个电机输出。

### 循迹

```c
typedef struct {
    float vx_mps;
    float wz_radps;
    bool line_valid;
    bool a_marker_event;
} LineFollow_Output_t;

bool LineFollow_Init(void);
void LineFollow_SetEnabled(bool enabled);
void LineFollow_Update(float dt_seconds);
void LineFollow_GetOutput(LineFollow_Output_t *output);
```

禁用时输出速度为零且不积累 PID 积分。线路无效时 `line_valid` 为假，速度建议必须为零；恢复识别后重新进入闭环，不能沿用失线期间的积分。

### 平衡机构

```c
typedef struct {
    float target_position;
    bool enabled;
} BallBalance_Command_t;

typedef struct {
    float measured_position;
    int32_t step_position;
    bool vision_valid;
    bool enabled;
    bool at_soft_limit;
} BallBalance_Status_t;

bool BallBalance_Init(void);
void BallBalance_SetCommand(const BallBalance_Command_t *command);
void BallBalance_Update(uint32_t now_ms, float dt_seconds);
void BallBalance_StepperService(void);
void BallBalance_Stop(void);
void BallBalance_GetStatus(BallBalance_Status_t *status);
```

`target_position` 与视觉测量使用同一协议坐标单位，协议确定后统一在 `ball_balance` 内转换。`BallBalance_Update()` 只在收到新且有效的视觉帧时更新位置外环，避免对同一帧重复累计。`BallBalance_StepperService()` 每 1 ms 调用一次 `Stepper_Task(device, 1U)`，与较慢的球位置外环分离。

### 比赛协调

```c
typedef enum {
    COMPETITION_DISARMED = 0,
    COMPETITION_READY,
    COMPETITION_RUNNING,
    COMPETITION_FINISHED,
    COMPETITION_FAULT,
} Competition_State_t;

typedef struct {
    Competition_State_t state;
    uint32_t elapsed_ms;
    bool line_valid;
    bool vision_valid;
} Competition_Status_t;

bool Competition_Init(void);
void Competition_Update(uint32_t now_ms);
void Competition_RequestStart(void);
void Competition_RequestStop(void);
void Competition_GetStatus(Competition_Status_t *status);
```

状态迁移由表驱动的事件处理函数完成，不使用一个不断膨胀的 `switch-case`。状态处理函数只组合上述公开接口，不持有 Motor、Stepper 或 GPIO 句柄。

## 5. 状态与数据流

```text
按键 / 比赛计时 / 安全状态
             |
             v
      Competition_Update
          |          |
          |          +--> BallBalance_SetCommand
          |
          +--> LineFollow_SetEnabled
                        |
                        v
              LineFollow_GetOutput
                        |
                        v
                Chassis_SetCommand
```

状态语义如下：

- `DISARMED`：上电默认状态。底盘停止，步进失能，等待机构人工置于机械中位。
- `READY`：初始化完成且允许启动，但两个执行系统仍不输出。启动请求进入 `RUNNING` 并清零比赛计时。
- `RUNNING`：根据赛题配置启用循迹和平衡控制。循迹输出转换为底盘命令；A 标志事件只由 `competition` 消费，用于阶段计数或计时，不由 `line_follow` 自行改变整车状态。
- `FINISHED`：时间或阶段条件满足后的锁定停止状态，必须收到新的启动流程才能再次运行。
- `FAULT`：不可恢复的初始化失败或执行机构故障。底盘停止、步进停止并失能；重新初始化前不自动恢复。

线路暂时丢失属于运行期保护：立即停止底盘，但保留 `RUNNING` 状态和比赛计时，线路稳定恢复后继续。视觉超时只停止并失能平衡执行机构，不应让底盘继续使用旧球位置，也不自动结束整场比赛。是否把重复超时升级为 `FAULT` 留给实车策略阶段，首版不做该升级。

## 6. 周期与调度

FreeRTOS 使用绝对周期延时，避免任务执行时间造成持续漂移。

| 工作 | 周期 | 所属 |
| --- | ---: | --- |
| 步进脉冲服务 | 1 ms | `BallBalance_StepperService()` |
| 灰度更新、循迹 PID、底盘闭环 | 5 ms | `RobotTask()` 控制路径 |
| 比赛状态和计时 | 5 ms | `Competition_Update()` |
| 球位置外环 | 10-20 ms 或新视觉帧 | `BallBalance_Update()` |
| OLED 状态显示 | 200 ms | 现有 OLED 任务 |
| UART 接收 | DMA/中断持续接收 | BSP/module |

首版保留一个 5 ms 整车控制任务，调用顺序固定为：采集输入、更新 `competition`、更新 `line_follow`、形成底盘命令、更新 `chassis`、按到期条件更新球位置外环。步进 1 ms 服务单独任务运行，不能塞进 5 ms 主循环。

`robot.c` 不直接拼 OLED 文本。OLED 任务读取各 app 的只读状态快照，显示比赛状态、用时、线路有效性、球位置、视觉有效性、左右轮目标与反馈。显示失败只增加诊断计数，不改变控制状态。

## 7. 安全与限幅

- 上电后底盘和步进均保持禁用，初始化顺序不能产生短时输出。
- 启用平衡前由操作者把机构放在约定中位，`BallBalance_Init()` 将该位置记为软件零点；没有限位开关前不宣称支持自动回零。
- 所有步进相对位移先经过单周期最大步数限制，再经过累计软件行程限制。
- 到达软件限位时只允许向离开限位的方向移动，并在状态中报告 `at_soft_limit`。
- 新的 `Stepper_Move()` 命令不能无条件覆盖正在执行的剩余步数。首版策略是仅在当前小步移动完成后接收下一次外环命令，防止控制周期不断重置运动。
- 视觉帧必须通过长度、字段范围和帧校验后才能刷新有效时间戳。超时阈值集中放在 `robot_def.h`。
- 循迹失线、显式停止、任务初始化失败都必须产生确定的零输出，不能依赖上一次命令自然过期。
- 电机方向参数继续同时反转驱动和编码器反馈，避免反向目标下 PID 朝错误方向追赶。

## 8. 参数归属

`robot_def.h` 按以下区域集中实车参数：

1. 比赛模式、时限和 A 标志去抖参数。
2. 循迹基础速度、最大角速度、灰度有效阈值和循迹 PID。
3. 底盘几何、编码器、左右轮速度 PID 和方向。
4. 视觉超时、目标球位置和数据范围。
5. 平衡外环 PID、死区、单次步数、步进速度和软件行程。

内部状态、设备句柄、计数器和计算中间量不放入 `robot_def.h`。参数宏使用单位后缀，例如 `_MS`、`_MPS`、`_RADPS`、`_STEPS`，避免依靠注释猜单位。

## 9. 迁移策略

1. 新增四个 app 的空实现和主机测试骨架，先冻结头文件接口。
2. 将现有 `chassis` 的 DRV8701E 注册和速度闭环迁入新接口，删除公开的 `chassis_manual_*` 全局变量。
3. 实现 `line_follow`，先用注入的灰度样本验证偏差、失线和 A 标志单次事件，再连接 `GraySensorInstance`。
4. 实现 `ball_balance` 的视觉帧状态、超时和有界步进命令，再连接尚未冻结的视觉协议解析。
5. 用 `competition` 替换 `app/cmd` 和 A/B 模板状态机；删除 `ROBOT_ENABLE_CMD_APP`，改为 H 题 app 启用开关。
6. 精简 `robot.c` 和 `robot_task.h`，加入独立 1 ms 步进服务任务，保持 OLED 200 ms 任务。
7. 默认硬件测试模式从 `HARDWARE_TEST_STEPPER_UART` 恢复为 `HARDWARE_TEST_NONE`，但保留所有单项硬件测试入口。

迁移过程中每一步都保持可构建，不一次删除旧接口后再补调用方。

## 10. 验证策略

### 主机测试

- `line_follow`：居中、左右偏差、全白/全黑失线、PID 禁用清零、A 标志去抖和单次事件。
- `chassis`：差速换算、使能/停止、左右方向一致性、命令复制和无效 `dt` 保护。
- `ball_balance`：新帧驱动、重复帧不积分、视觉超时、死区、单步限幅、累计软限位、禁用清零。
- `competition`：每个合法迁移、停止优先级、完成锁定、失线保护、视觉超时降级和 A 标志消费。
- `robot`：硬件测试模式隔离、初始化失败保持安全、5 ms 与 1 ms 调度入口存在。

### 构建验证

- 运行全部主机测试。
- 生成 SysConfig 并构建 FreeRTOS 固件。
- 构建 NoRTOS 固件，保证 BSP/module 仍可复用。
- 检查链接映射，确认旧 `app/cmd` 不再进入固件。

### 硬件验证顺序

1. 保持底盘断电，验证上电和烧录期间 STEP、DIR、EN 与电机 PWM 均为安全电平。
2. 单独验证步进方向、软件零点、单步限幅和两端软限位。
3. 单独验证视觉帧、坐标方向、丢帧和超时停机。
4. 架空车轮验证左右编码器方向、目标速度和停止行为。
5. 低速落地循迹，验证失线停止和 A 标志只触发一次。
6. 固定车辆调平衡外环，再进行循迹与平衡联合测试。
7. 最后验证比赛启动、计时、完成和紧急停止全流程。

## 11. 明确不做的事项

- 不在协议确定前设计复杂视觉消息格式。
- 不在缺少原点或限位传感器时实现伪自动回零。
- 不引入动态注册、解绑流程或消息总线。
- 不把 OLED、VOFA 或 Live Watch 调试变量作为控制链路依赖。
- 不在本轮重构未使用的 SPI、云台、发射和旧 RM 兼容类型。
