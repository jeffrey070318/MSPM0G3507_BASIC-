#ifndef MODULE_STEPPER_H
#define MODULE_STEPPER_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STEPPER_DEFAULT_PULSE_WIDTH_US 5U
#define STEPPER_MIN_SPEED_SPS          1U
#define STEPPER_MAX_SPEED_SPS          2000U
#define STEPPER_TASK_PERIOD_MS         1U

typedef enum {
    STEPPER_DIR_UP = 0,
    STEPPER_DIR_DOWN,
} Stepper_Direction_e;

typedef struct {
    bool initialized;
    bool enabled;
    bool running;
    bool enable_active_low;
    Stepper_Direction_e direction;
    uint16_t pulse_width_us;
    uint16_t speed_sps;
    uint32_t accumulator_milli_steps;
    uint32_t emitted_steps;
    int32_t remaining_steps;
    int32_t position_steps;
} Stepper_Device_t;

/* 初始化 STEP/DIR/EN 输出，默认禁用电机并让 STEP 保持空闲高电平。 */
Device_Status_e Stepper_Init(Stepper_Device_t *device);
/* 控制驱动器 EN 脚；当前默认按低有效使能处理。 */
Device_Status_e Stepper_Enable(Stepper_Device_t *device, bool enable);
/* 启动一次相对移动，方向和步数由上层闭环或串口命令给出。 */
Device_Status_e Stepper_Move(Stepper_Device_t *device,
    Stepper_Direction_e direction, uint32_t steps, uint16_t speed_sps);
/* 停止当前未完成的相对移动，不改变已累计的位置。 */
void Stepper_Stop(Stepper_Device_t *device);
/* Reset the software position estimate after a manual level/home calibration. */
void Stepper_ResetPosition(Stepper_Device_t *device, int32_t position_steps);
/* 1 ms 周期调用，按 speed_sps 累加并输出 STEP 低脉冲。 */
void Stepper_Task(Stepper_Device_t *device, uint16_t elapsed_ms);

#ifdef __cplusplus
}
#endif

#endif
