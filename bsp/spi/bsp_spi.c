#include "bsp_spi.h"
#include "bsp_memory.h"

#include "memory.h"

#define SPI_POLL_LIMIT (1000000U)
#define SPI_ERROR_MASK                                                     \
    (DL_SPI_INTERRUPT_RX_TIMEOUT | DL_SPI_INTERRUPT_PARITY_ERROR |         \
        DL_SPI_INTERRUPT_RX_OVERFLOW)

static SPIInstance *spi_instance[MX_SPI_BUS_SLAVE_CNT] = {NULL};
static uint8_t idx = 0;
uint8_t SPIDeviceOnGoing[SPI_DEVICE_CNT] = {1};

static bool SPIHasError(SPI_Regs *spi)
{
    uint32_t status = DL_SPI_getRawInterruptStatus(spi, SPI_ERROR_MASK);
    if (status == 0U) {
        return false;
    }

    DL_SPI_clearInterruptStatus(spi, status);
    return true;
}

static void SPIDrainRXFIFO(SPI_Regs *spi)
{
    while (!DL_SPI_isRXFIFOEmpty(spi)) {
        (void) DL_SPI_receiveData8(spi);
    }
}

static bool SPIAcquireBus(SPIInstance *spi_ins)
{
    if ((spi_ins == NULL) || (spi_ins->cs_pin_state == NULL)) {
        return false;
    }

    for (uint32_t poll = 0U; poll < SPI_POLL_LIMIT; ++poll) {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (*spi_ins->cs_pin_state != 0U) {
            *spi_ins->cs_pin_state = 0U;
            if (primask == 0U) {
                __enable_irq();
            }
            return true;
        }
        if (primask == 0U) {
            __enable_irq();
        }
    }

    return false;
}

static void SPIReleaseBus(SPIInstance *spi_ins)
{
    if ((spi_ins != NULL) && (spi_ins->cs_pin_state != NULL)) {
        *spi_ins->cs_pin_state = 1U;
    }
}

static bool SPITransferByte(
    SPIInstance *spi_ins, uint8_t tx_data, uint8_t *rx_data)
{
    if ((spi_ins == NULL) || (spi_ins->spi_handle == NULL) ||
        (spi_ins->spi_handle->Instance == NULL) ||
        (rx_data == NULL)) {
        return false;
    }

    SPI_Regs *spi = spi_ins->spi_handle->Instance;

    uint32_t poll = 0U;
    while (DL_SPI_isTXFIFOFull(spi)) {
        if (SPIHasError(spi) || (++poll >= SPI_POLL_LIMIT)) {
            return false;
        }
    }
    DL_SPI_transmitData8(spi, tx_data);

    poll = 0U;
    while (DL_SPI_isRXFIFOEmpty(spi)) {
        if (SPIHasError(spi) || (++poll >= SPI_POLL_LIMIT)) {
            return false;
        }
    }

    *rx_data = DL_SPI_receiveData8(spi);
    return true;
}

static bool SPIAssertCS(SPIInstance *spi_ins)
{
    if ((spi_ins == NULL) || (spi_ins->spi_handle == NULL) ||
        (spi_ins->spi_handle->Instance == NULL) ||
        (spi_ins->GPIOx == NULL) ||
        (spi_ins->cs_pin == 0U)) {
        return false;
    }

    SPI_Regs *spi = spi_ins->spi_handle->Instance;
    DL_SPI_clearInterruptStatus(spi, SPI_ERROR_MASK);
    SPIDrainRXFIFO(spi);
    DL_GPIO_clearPins(spi_ins->GPIOx, spi_ins->cs_pin);
    spi_ins->CS_State = 0U;
    return true;
}

static bool SPIReleaseCS(SPIInstance *spi_ins)
{
    if ((spi_ins == NULL) || (spi_ins->spi_handle == NULL) ||
        (spi_ins->spi_handle->Instance == NULL) ||
        (spi_ins->GPIOx == NULL) ||
        (spi_ins->cs_pin == 0U)) {
        return false;
    }

    SPI_Regs *spi = spi_ins->spi_handle->Instance;
    bool complete = false;

    for (uint32_t poll = 0U; poll < SPI_POLL_LIMIT; ++poll) {
        if (SPIHasError(spi)) {
            break;
        }
        if (!DL_SPI_isBusy(spi)) {
            complete = true;
            break;
        }
    }

    DL_GPIO_setPins(spi_ins->GPIOx, spi_ins->cs_pin);
    spi_ins->CS_State = 1U;
    return complete;
}

SPIInstance *SPIRegister(SPI_Init_Config_s *conf)
{
    if ((conf == NULL) || (conf->spi_handle == NULL) ||
        (conf->spi_handle->Instance != SPI_0_INST) ||
        (conf->GPIOx == NULL) || (conf->cs_pin == 0U) ||
        (idx >= MX_SPI_BUS_SLAVE_CNT)) {
        return NULL;
    }

    SPIInstance *instance =
        (SPIInstance *) BSPMalloc(sizeof(SPIInstance));
    if (instance == NULL) {
        return NULL;
    }
    memset(instance, 0, sizeof(SPIInstance));

    instance->spi_handle = conf->spi_handle;
    instance->GPIOx = conf->GPIOx;
    instance->cs_pin = conf->cs_pin;
    instance->spi_work_mode = conf->spi_work_mode;
    instance->callback = conf->callback;
    instance->id = conf->id;

    instance->cs_pin_state = &SPIDeviceOnGoing[0];
    *instance->cs_pin_state = 1U;
    instance->CS_State = 1U;
    DL_GPIO_setPins(instance->GPIOx, instance->cs_pin);

    spi_instance[idx++] = instance;
    return instance;
}

void SPITransmit(SPIInstance *spi_ins, uint8_t *ptr_data, uint8_t len)
{
    if ((spi_ins == NULL) || (ptr_data == NULL) || (len == 0U) ||
        !SPIAcquireBus(spi_ins)) {
        return;
    }

    bool success = SPIAssertCS(spi_ins);
    for (uint8_t i = 0U; i < len; ++i) {
        uint8_t rx_data;
        if (!success || !SPITransferByte(spi_ins, ptr_data[i], &rx_data)) {
            success = false;
            break;
        }
    }
    success = SPIReleaseCS(spi_ins) && success;
    SPIReleaseBus(spi_ins);

    if (success && (spi_ins->spi_work_mode != SPI_BLOCK_MODE) &&
        (spi_ins->callback != NULL)) {
        spi_ins->callback(spi_ins);
    }
}

void SPIRecv(SPIInstance *spi_ins, uint8_t *ptr_data, uint8_t len)
{
    if ((spi_ins == NULL) || (ptr_data == NULL) || (len == 0U) ||
        !SPIAcquireBus(spi_ins)) {
        return;
    }

    spi_ins->rx_size = len;
    spi_ins->rx_buffer = ptr_data;

    bool success = SPIAssertCS(spi_ins);
    for (uint8_t i = 0U; i < len; ++i) {
        if (!success || !SPITransferByte(spi_ins, 0xFFU, &ptr_data[i])) {
            success = false;
            break;
        }
    }
    success = SPIReleaseCS(spi_ins) && success;
    SPIReleaseBus(spi_ins);

    if (success && (spi_ins->callback != NULL)) {
        spi_ins->callback(spi_ins);
    }
}

void SPITransRecv(
    SPIInstance *spi_ins, uint8_t *ptr_data_rx, uint8_t *ptr_data_tx, uint8_t len)
{
    if ((spi_ins == NULL) || (ptr_data_rx == NULL) ||
        (ptr_data_tx == NULL) || (len == 0U) || !SPIAcquireBus(spi_ins)) {
        return;
    }

    spi_ins->rx_size = len;
    spi_ins->rx_buffer = ptr_data_rx;

    bool success = SPIAssertCS(spi_ins);
    for (uint8_t i = 0U; i < len; ++i) {
        if (!success ||
            !SPITransferByte(spi_ins, ptr_data_tx[i], &ptr_data_rx[i])) {
            success = false;
            break;
        }
    }
    success = SPIReleaseCS(spi_ins) && success;
    SPIReleaseBus(spi_ins);

    if (success && (spi_ins->callback != NULL)) {
        spi_ins->callback(spi_ins);
    }
}

void SPISetMode(SPIInstance *spi_ins, SPI_TXRX_MODE_e spi_mode)
{
    if (spi_mode != SPI_DMA_MODE && spi_mode != SPI_IT_MODE &&
        spi_mode != SPI_BLOCK_MODE) {
        return;
    }

    if (spi_ins != NULL) {
        spi_ins->spi_work_mode = spi_mode;
    }
}
