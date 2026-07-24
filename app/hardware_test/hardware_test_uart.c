#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_UART

#include "framework_runtime.h"
#include "vofa.h"

volatile uint32_t hardware_test_uart_send_count;
volatile uint32_t hardware_test_uart_busy_count;
volatile Device_Status_e hardware_test_uart_last_status = DEVICE_ERROR;

Device_Status_e HardwareTestInit(void)
{
    return VOFA_Init();
}

void HardwareTestRun(void)
{
    float vofa_data[3] = {
        (float) hardware_test_uart_send_count,
        (float) framework_robot_heartbeat,
        3507.0f,
    };

    hardware_test_uart_last_status =
        VOFA_JustFloatOutputDMA(vofa_data, 3U);
    if (hardware_test_uart_last_status == DEVICE_OK) {
        hardware_test_uart_send_count++;
    } else if (hardware_test_uart_last_status == DEVICE_BUSY) {
        hardware_test_uart_busy_count++;
    }
}

#endif
