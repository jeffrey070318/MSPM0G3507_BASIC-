#include "bsp_spi.h"

#include "memory.h"
#include "stdlib.h"

static SPIInstance *spi_instance[SPI_DEVICE_CNT] = {NULL};
static uint8_t idx = 0;
uint8_t SPIDeviceOnGoing[SPI_DEVICE_CNT] = {1};

static uint8_t SPITransferByte(SPIInstance *spi_ins, uint8_t tx_data)
{
    SPI_Regs *spi = spi_ins->spi_handle->Instance;

    while (DL_SPI_isTXFIFOFull(spi)) {
    }
    DL_SPI_transmitData8(spi, tx_data);

    while (DL_SPI_isRXFIFOEmpty(spi)) {
    }

    return DL_SPI_receiveData8(spi);
}

static void SPIAssertCS(SPIInstance *spi_ins)
{
    HAL_GPIO_WritePin(spi_ins->GPIOx, spi_ins->cs_pin, GPIO_PIN_RESET);
    *spi_ins->cs_pin_state = 0U;
    spi_ins->CS_State = 0U;
}

static void SPIReleaseCS(SPIInstance *spi_ins)
{
    while (DL_SPI_isBusy(spi_ins->spi_handle->Instance)) {
    }

    HAL_GPIO_WritePin(spi_ins->GPIOx, spi_ins->cs_pin, GPIO_PIN_SET);
    *spi_ins->cs_pin_state = 1U;
    spi_ins->CS_State = 1U;
}

SPIInstance *SPIRegister(SPI_Init_Config_s *conf)
{
    if (idx >= MX_SPI_BUS_SLAVE_CNT) {
        while (1) {
        }
    }

    SPIInstance *instance = (SPIInstance *) malloc(sizeof(SPIInstance));
    memset(instance, 0, sizeof(SPIInstance));

    instance->spi_handle = conf->spi_handle;
    instance->GPIOx = conf->GPIOx;
    instance->cs_pin = conf->cs_pin;
    instance->spi_work_mode = conf->spi_work_mode;
    instance->callback = conf->callback;
    instance->id = conf->id;

    if ((instance->spi_handle == NULL) ||
        (instance->spi_handle->Instance != SPI_0_INST)) {
        while (1) {
        }
    }

    instance->cs_pin_state = &SPIDeviceOnGoing[0];
    *instance->cs_pin_state = 1U;

    spi_instance[idx++] = instance;
    return instance;
}

void SPITransmit(SPIInstance *spi_ins, uint8_t *ptr_data, uint8_t len)
{
    if ((spi_ins == NULL) || (ptr_data == NULL)) {
        return;
    }

    SPIAssertCS(spi_ins);
    for (uint8_t i = 0U; i < len; ++i) {
        (void) SPITransferByte(spi_ins, ptr_data[i]);
    }
    SPIReleaseCS(spi_ins);

    if ((spi_ins->spi_work_mode != SPI_BLOCK_MODE) && (spi_ins->callback != NULL)) {
        spi_ins->callback(spi_ins);
    }
}

void SPIRecv(SPIInstance *spi_ins, uint8_t *ptr_data, uint8_t len)
{
    if ((spi_ins == NULL) || (ptr_data == NULL)) {
        return;
    }

    spi_ins->rx_size = len;
    spi_ins->rx_buffer = ptr_data;

    SPIAssertCS(spi_ins);
    for (uint8_t i = 0U; i < len; ++i) {
        ptr_data[i] = SPITransferByte(spi_ins, 0xFFU);
    }
    SPIReleaseCS(spi_ins);

    if (spi_ins->callback != NULL) {
        spi_ins->callback(spi_ins);
    }
}

void SPITransRecv(
    SPIInstance *spi_ins, uint8_t *ptr_data_rx, uint8_t *ptr_data_tx, uint8_t len)
{
    if ((spi_ins == NULL) || (ptr_data_rx == NULL) || (ptr_data_tx == NULL)) {
        return;
    }

    spi_ins->rx_size = len;
    spi_ins->rx_buffer = ptr_data_rx;

    while (!SPIDeviceOnGoing[0]) {
    }

    SPIAssertCS(spi_ins);
    for (uint8_t i = 0U; i < len; ++i) {
        ptr_data_rx[i] = SPITransferByte(spi_ins, ptr_data_tx[i]);
    }
    SPIReleaseCS(spi_ins);

    if (spi_ins->callback != NULL) {
        spi_ins->callback(spi_ins);
    }
}

void SPISetMode(SPIInstance *spi_ins, SPI_TXRX_MODE_e spi_mode)
{
    if (spi_mode != SPI_DMA_MODE && spi_mode != SPI_IT_MODE &&
        spi_mode != SPI_BLOCK_MODE) {
        while (1) {
        }
    }

    if (spi_ins != NULL) {
        spi_ins->spi_work_mode = spi_mode;
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    for (size_t i = 0; i < idx; i++) {
        if (spi_instance[i]->spi_handle == hspi &&
            spi_instance[i]->callback != NULL) {
            spi_instance[i]->callback(spi_instance[i]);
            return;
        }
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    HAL_SPI_RxCpltCallback(hspi);
}
