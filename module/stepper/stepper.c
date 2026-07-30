#include "stepper.h"

#include <limits.h>

#include "bsp_dwt.h"

#define STEPPER_MAX_PULSES_PER_TASK 4U

/* 将速度限制在驱动器和软件调度能稳定处理的范围内。 */
static uint16_t StepperClampSpeed(uint16_t speed_sps)
{
    if (speed_sps < STEPPER_MIN_SPEED_SPS) {
        return STEPPER_MIN_SPEED_SPS;
    }
    if (speed_sps > STEPPER_MAX_SPEED_SPS) {
        return STEPPER_MAX_SPEED_SPS;
    }
    return speed_sps;
}

/* 根据使能极性输出 EN 脚，适配张大头闭环电机常见低有效使能。 */
static void StepperApplyEnable(const Stepper_Device_t *device, bool enable)
{
    const bool active_level =
        device->enable_active_low ? !enable : enable;
    if (active_level) {
        DL_GPIO_setPins(STEPPER_GPIO_PORT, STEPPER_GPIO_EN_PIN);
    } else {
        DL_GPIO_clearPins(STEPPER_GPIO_PORT, STEPPER_GPIO_EN_PIN);
    }
}

/* 输出方向脚；若实际上下方向相反，可调换这里的高低电平。 */
static void StepperApplyDirection(Stepper_Direction_e direction)
{
    if (direction == STEPPER_DIR_UP) {
        DL_GPIO_setPins(STEPPER_GPIO_PORT, STEPPER_GPIO_DIR_PIN);
    } else {
        DL_GPIO_clearPins(STEPPER_GPIO_PORT, STEPPER_GPIO_DIR_PIN);
    }
}

/* 输出一个 STEP 低脉冲；COM 接 3.3V 时驱动器通常识别低脉冲。 */
static void StepperPulse(const Stepper_Device_t *device)
{
    DL_GPIO_clearPins(STEPPER_GPIO_PORT, STEPPER_GPIO_STEP_PIN);
    DWT_Delay((float) device->pulse_width_us * 0.000001f);
    DL_GPIO_setPins(STEPPER_GPIO_PORT, STEPPER_GPIO_STEP_PIN);
}

/* 初始化软件状态和 GPIO 空闲态，避免上电误动作。 */
Device_Status_e Stepper_Init(Stepper_Device_t *device)
{
    if (device == NULL) {
        return DEVICE_ERROR;
    }

    device->initialized = true;
    device->enabled = false;
    device->running = false;
    device->enable_active_low = true;
    device->direction = STEPPER_DIR_UP;
    device->pulse_width_us = STEPPER_DEFAULT_PULSE_WIDTH_US;
    device->speed_sps = 0U;
    device->accumulator_milli_steps = 0U;
    device->emitted_steps = 0U;
    device->remaining_steps = 0;
    device->position_steps = 0;

    DL_GPIO_setPins(STEPPER_GPIO_PORT, STEPPER_GPIO_STEP_PIN);
    DL_GPIO_clearPins(STEPPER_GPIO_PORT, STEPPER_GPIO_DIR_PIN);
    StepperApplyEnable(device, false);
    return DEVICE_OK;
}

/* 软件使能/失能电机；失能时同时清掉未完成移动。 */
Device_Status_e Stepper_Enable(Stepper_Device_t *device, bool enable)
{
    if ((device == NULL) || !device->initialized) {
        return DEVICE_ERROR;
    }

    device->enabled = enable;
    if (!enable) {
        device->running = false;
        device->remaining_steps = 0;
        device->accumulator_milli_steps = 0U;
    }
    StepperApplyEnable(device, enable);
    return DEVICE_OK;
}

/* 配置一次有限步数移动，实际脉冲由 Stepper_Task 周期输出。 */
Device_Status_e Stepper_Move(Stepper_Device_t *device,
    Stepper_Direction_e direction, uint32_t steps, uint16_t speed_sps)
{
    if ((device == NULL) || !device->initialized || (steps == 0U) ||
        (steps > (uint32_t) INT32_MAX)) {
        return DEVICE_ERROR;
    }

    device->direction = direction;
    device->speed_sps = StepperClampSpeed(speed_sps);
    device->remaining_steps = (int32_t) steps;
    device->accumulator_milli_steps = 0U;
    device->running = true;
    (void) Stepper_Enable(device, true);
    StepperApplyDirection(direction);
    return DEVICE_OK;
}

/* 停止当前运动，用于串口 STOP 或闭环保护。 */
void Stepper_Stop(Stepper_Device_t *device)
{
    if (device == NULL) {
        return;
    }

    device->running = false;
    device->remaining_steps = 0;
    device->accumulator_milli_steps = 0U;
}

/* 周期任务：把 steps/s 转成按毫秒累加的脉冲输出。 */
void Stepper_Task(Stepper_Device_t *device, uint16_t elapsed_ms)
{
    if ((device == NULL) || !device->initialized || !device->enabled ||
        !device->running || (device->speed_sps == 0U) ||
        (elapsed_ms == 0U)) {
        return;
    }

    device->accumulator_milli_steps +=
        (uint32_t) device->speed_sps * (uint32_t) elapsed_ms;

    uint8_t pulses = 0U;
    while ((device->accumulator_milli_steps >= 1000U) &&
           (pulses < STEPPER_MAX_PULSES_PER_TASK)) {
        if (device->remaining_steps <= 0) {
            Stepper_Stop(device);
            break;
        }

        device->accumulator_milli_steps -= 1000U;
        StepperPulse(device);
        device->emitted_steps++;
        device->position_steps +=
            (device->direction == STEPPER_DIR_UP) ? 1 : -1;
        device->remaining_steps--;
        pulses++;
    }
}
