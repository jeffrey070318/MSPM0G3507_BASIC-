#include "vofa.h"

#include <string.h>

#include "bsp_usart.h"

static USARTInstance *vofa_usart;

Device_Status_e VOFA_Init(void)
{
    if (vofa_usart != NULL) return DEVICE_OK;

    USART_Init_Config_s config = {
        .recv_buff_size = 16U,
        .usart_handle = &huart1,
        .module_callback = NULL,
    };
    vofa_usart = USARTRegister(&config);
    return (vofa_usart != NULL) ? DEVICE_OK : DEVICE_ERROR;
}

static Device_Status_e VOFABuildAndSend(
    const float *data, uint8_t num, USART_TRANSFER_MODE mode)
{
    if ((data == NULL) || (vofa_usart == NULL) || (num == 0U) ||
        (num > VOFA_JUSTFLOAT_MAX_NUM)) {
        return DEVICE_ERROR;
    }

    uint8_t send_data[4U * VOFA_JUSTFLOAT_MAX_NUM + 4U];
    uint16_t payload_size = (uint16_t) num * sizeof(float);
    memcpy(send_data, data, payload_size);
    send_data[payload_size] = 0x00U;
    send_data[payload_size + 1U] = 0x00U;
    send_data[payload_size + 2U] = 0x80U;
    send_data[payload_size + 3U] = 0x7FU;

    return USARTSendEx(vofa_usart, send_data,
        (uint16_t) (payload_size + 4U), mode);
}

Device_Status_e VOFA_JustFloatOutput(const float *data, uint8_t num)
{
    return VOFABuildAndSend(data, num, USART_TRANSFER_BLOCKING);
}

Device_Status_e VOFA_JustFloatOutputDMA(const float *data, uint8_t num)
{
    return VOFABuildAndSend(data, num, USART_TRANSFER_DMA);
}
