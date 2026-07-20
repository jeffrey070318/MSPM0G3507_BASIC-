#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_UART

#include "bsp_usart.h"
#include "framework_runtime.h"
#include "vofa.h"

volatile uint32_t hardware_test_uart_send_count;
volatile uint32_t hardware_test_uart_busy_count;
volatile Device_Status_e hardware_test_uart_last_status = DEVICE_ERROR;

static USARTInstance *hardware_test_uart;

Device_Status_e HardwareTestInit(void)
{
    USART_Init_Config_s uart_config = {
        .recv_buff_size = 16U,
        .usart_handle = &huart1,
        .module_callback = NULL,
    };

    hardware_test_uart = USARTRegister(&uart_config);
    return (hardware_test_uart != NULL) ? DEVICE_OK : DEVICE_ERROR;
}

void HardwareTestRun(void)
{
    float vofa_data[3] = {
        (float) hardware_test_uart_send_count,
        (float) framework_robot_heartbeat,
        3507.0f,
    };

    hardware_test_uart_last_status =
        vofa_justfloat_output_dma(vofa_data, 3U, &huart1);
    if (hardware_test_uart_last_status == DEVICE_OK) {
        hardware_test_uart_send_count++;
    } else if (hardware_test_uart_last_status == DEVICE_BUSY) {
        hardware_test_uart_busy_count++;
    }
}

#endif
