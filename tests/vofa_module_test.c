#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "bsp_usart.h"
#include "vofa.h"

UART_HandleTypeDef huart1;

static USARTInstance usart_instance;
static USART_Init_Config_s captured_config;
static uint8_t captured_data[32];
static uint16_t captured_size;
static USART_TRANSFER_MODE captured_mode;

USARTInstance *USARTRegister(USART_Init_Config_s *config)
{
    assert(config != NULL);
    captured_config = *config;
    return &usart_instance;
}

Device_Status_e USARTSendEx(USARTInstance *instance, uint8_t *data,
    uint16_t size, USART_TRANSFER_MODE mode)
{
    assert(instance == &usart_instance);
    assert(size <= sizeof(captured_data));
    memcpy(captured_data, data, size);
    captured_size = size;
    captured_mode = mode;
    return DEVICE_OK;
}

int main(void)
{
    assert(VOFA_Init() == DEVICE_OK);
    assert(captured_config.usart_handle == &huart1);
    assert(captured_config.recv_buff_size == 16U);
    assert(captured_config.module_callback == NULL);

    const float values[2] = {1.0f, -2.0f};
    assert(VOFA_JustFloatOutputDMA(values, 2U) == DEVICE_OK);
    assert(captured_mode == USART_TRANSFER_DMA);
    assert(captured_size == 12U);
    assert(memcmp(captured_data, values, sizeof(values)) == 0);
    assert(captured_data[8] == 0x00U);
    assert(captured_data[9] == 0x00U);
    assert(captured_data[10] == 0x80U);
    assert(captured_data[11] == 0x7FU);

    assert(VOFA_JustFloatOutput(values, 2U) == DEVICE_OK);
    assert(captured_mode == USART_TRANSFER_BLOCKING);
    assert(VOFA_JustFloatOutput(NULL, 2U) == DEVICE_ERROR);
    assert(VOFA_JustFloatOutput(values, 0U) == DEVICE_ERROR);
    return 0;
}
