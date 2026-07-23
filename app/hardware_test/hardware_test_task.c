#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE != HARDWARE_TEST_NONE

#include "FreeRTOS.h"
#include "task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_GPIO
#define HARDWARE_TEST_PERIOD_MS 500U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_PWM
#define HARDWARE_TEST_PERIOD_MS 1000U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_UART
#define HARDWARE_TEST_PERIOD_MS 100U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_GRAY_SENSOR
#define HARDWARE_TEST_PERIOD_MS 10U
#elif HARDWARE_TEST_MODE == HARDWARE_TEST_ENCODER
#define HARDWARE_TEST_PERIOD_MS 10U
#else
#define HARDWARE_TEST_PERIOD_MS 1000U
#endif

volatile Device_Status_e hardware_test_init_status = DEVICE_ERROR;
volatile uint32_t hardware_test_run_count;

__attribute__((noreturn)) void StartHARDWARETESTTASK(void *argument)
{
    (void) argument;

    hardware_test_init_status = HardwareTestInit();

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks =
        pdMS_TO_TICKS(HARDWARE_TEST_PERIOD_MS);

    for (;;) {
        if (hardware_test_init_status == DEVICE_OK) {
            HardwareTestRun();
            hardware_test_run_count++;
        }
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}

#endif
