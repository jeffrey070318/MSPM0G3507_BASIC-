#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE != HARDWARE_TEST_NONE

#include "FreeRTOS.h"
#include "task.h"

#include "framework_runtime.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_GPIO
#define HARDWARE_TEST_PERIOD_MS 500U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_PWM
#define HARDWARE_TEST_PERIOD_MS 1000U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_UART
#define HARDWARE_TEST_PERIOD_MS 100U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_STEPPER_UART
#include "stepper.h"
#define HARDWARE_TEST_PERIOD_MS STEPPER_TASK_PERIOD_MS
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_GRAY_SENSOR
#define HARDWARE_TEST_PERIOD_MS 10U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_ENCODER
#define HARDWARE_TEST_PERIOD_MS 10U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_MOTOR
#define HARDWARE_TEST_PERIOD_MS 10U
#else
#define HARDWARE_TEST_PERIOD_MS 1000U
#endif

volatile Device_Status_e hardware_test_init_status = DEVICE_ERROR;
volatile uint32_t hardware_test_task_started;
volatile uint32_t hardware_test_run_count;
volatile uint32_t hardware_test_idle_count;

static void HardwareTestHeartbeat(void)
{
    static uint16_t heartbeat_ticks;
    const uint16_t heartbeat_period =
        (uint16_t) (500U / HARDWARE_TEST_PERIOD_MS);

    heartbeat_ticks++;
    if (heartbeat_ticks >= heartbeat_period) {
        heartbeat_ticks = 0U;
        DL_GPIO_togglePins(LED_GPIO_PORT, LED_GPIO_BOARD_LED_PIN);
    }
}

__attribute__((noreturn)) void StartHARDWARETESTTASK(void *argument)
{
    (void) argument;

    framework_boot_stage = FRAMEWORK_BOOT_SCHEDULER_RUNNING;
    hardware_test_task_started = 1U;
    hardware_test_init_status = HardwareTestInit();

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks =
        pdMS_TO_TICKS(HARDWARE_TEST_PERIOD_MS);

    for (;;) {
        if (hardware_test_init_status == DEVICE_OK) {
            HardwareTestRun();
            hardware_test_run_count++;
        } else {
            hardware_test_idle_count++;
        }
        HardwareTestHeartbeat();
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}

#endif
