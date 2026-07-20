#include "vofa.h"

#include <string.h>

static Device_Status_e VOFABuildAndSend(const float *data, uint8_t num,
    UART_HandleTypeDef *huart, USART_TRANSFER_MODE mode)
{
    if ((data == NULL) || (huart == NULL) || (num == 0U) ||
        (num > VOFA_JUSTFLOAT_MAX_NUM)) {
        return DEVICE_ERROR;
    }

    USARTInstance *instance = USARTGetInstance(huart);
    if (instance == NULL) {
        return DEVICE_ERROR;
    }

    uint8_t send_data[4U * VOFA_JUSTFLOAT_MAX_NUM + 4U];
    uint16_t payload_size = (uint16_t) num * sizeof(float);
    memcpy(send_data, data, payload_size);
    send_data[payload_size] = 0x00U;
    send_data[payload_size + 1U] = 0x00U;
    send_data[payload_size + 2U] = 0x80U;
    send_data[payload_size + 3U] = 0x7FU;

    return USARTSendEx(instance, send_data,
        (uint16_t) (payload_size + 4U), mode);
}

void vofa_justfloat_output(
    float *data, uint8_t num, UART_HandleTypeDef *huart)
{
    (void) VOFABuildAndSend(
        data, num, huart, USART_TRANSFER_BLOCKING);
}

Device_Status_e vofa_justfloat_output_dma(
    const float *data, uint8_t num, UART_HandleTypeDef *huart)
{
    return VOFABuildAndSend(data, num, huart, USART_TRANSFER_DMA);
}
