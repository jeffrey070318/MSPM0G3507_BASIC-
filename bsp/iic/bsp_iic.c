#include "bsp_iic.h"
#include "bsp_memory.h"

#include "memory.h"
#include "stdlib.h"

#define IIC_POLL_LIMIT (1000000U)
#define IIC_FIFO_SIZE  (8U)
#define IIC_START_DELAY_CYCLES (32U)

static uint8_t idx = 0;
static IICInstance *iic_instance[MX_IIC_SLAVE_CNT] = {NULL};

static void IICRecordStatus(IICInstance *iic)
{
    if ((iic != NULL) && (iic->handle != NULL) &&
        (iic->handle->Instance != NULL)) {
        iic->controller_status =
            DL_I2C_getControllerStatus(iic->handle->Instance);
    }
}

static uint32_t IICAddress7(uint8_t dev_address)
{
    return (dev_address > 0x7FU) ? ((uint32_t) dev_address >> 1U)
                                 : (uint32_t) dev_address;
}

static Device_Status_e IICWaitIdle(I2C_Regs *i2c)
{
    for (uint32_t poll = 0U; poll < IIC_POLL_LIMIT; ++poll) {
        uint32_t status = DL_I2C_getControllerStatus(i2c);

        if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
            return DEVICE_ERROR;
        }
        if ((status & DL_I2C_CONTROLLER_STATUS_IDLE) != 0U) {
            return DEVICE_OK;
        }
    }

    return DEVICE_TIMEOUT;
}

static Device_Status_e IICWaitTransfer(I2C_Regs *i2c)
{
    for (uint32_t poll = 0U; poll < IIC_POLL_LIMIT; ++poll) {
        uint32_t status = DL_I2C_getControllerStatus(i2c);

        if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
            return DEVICE_ERROR;
        }
        if ((status & DL_I2C_CONTROLLER_STATUS_BUSY) == 0U) {
            return DEVICE_OK;
        }
    }

    return DEVICE_TIMEOUT;
}

static Device_Status_e IICBlockingTransmitEx(
    IICInstance *iic, uint8_t *data, uint16_t size, bool stop_enable,
    bool wait_idle)
{
    if ((iic == NULL) || (iic->handle == NULL) ||
        (iic->handle->Instance == NULL) ||
        (data == NULL) || (size == 0U)) {
        return DEVICE_ERROR;
    }

    I2C_Regs *i2c = iic->handle->Instance;
    if (wait_idle) {
        Device_Status_e idle_status = IICWaitIdle(i2c);
        if (idle_status != DEVICE_OK) {
            IICRecordStatus(iic);
            return idle_status;
        }
    }

    uint16_t initial_size =
        (size < IIC_FIFO_SIZE) ? size : IIC_FIFO_SIZE;
    DL_I2C_fillControllerTXFIFO(i2c, data, initial_size);

    DL_I2C_startControllerTransferAdvanced(i2c, IICAddress7(iic->dev_address),
        DL_I2C_CONTROLLER_DIRECTION_TX, size, DL_I2C_CONTROLLER_START_ENABLE,
        stop_enable ? DL_I2C_CONTROLLER_STOP_ENABLE
                     : DL_I2C_CONTROLLER_STOP_DISABLE,
        DL_I2C_CONTROLLER_ACK_DISABLE);
    delay_cycles(IIC_START_DELAY_CYCLES);

    for (uint16_t i = initial_size; i < size; ++i) {
        uint32_t poll = 0U;
        while (DL_I2C_isControllerTXFIFOFull(i2c)) {
            if ((DL_I2C_getControllerStatus(i2c) &
                    DL_I2C_CONTROLLER_STATUS_ERROR) != 0U ||
                ++poll >= IIC_POLL_LIMIT) {
                IICRecordStatus(iic);
                DL_I2C_resetControllerTransfer(i2c);
                return (poll >= IIC_POLL_LIMIT) ? DEVICE_TIMEOUT
                                                : DEVICE_ERROR;
            }
        }
        DL_I2C_transmitControllerData(i2c, data[i]);
    }

    Device_Status_e transfer_status = IICWaitTransfer(i2c);
    IICRecordStatus(iic);
    if (transfer_status != DEVICE_OK) {
        DL_I2C_resetControllerTransfer(i2c);
        return transfer_status;
    }

    return DEVICE_OK;
}

static Device_Status_e IICBlockingReceiveEx(
    IICInstance *iic, uint8_t *data, uint16_t size, bool wait_idle)
{
    if ((iic == NULL) || (iic->handle == NULL) ||
        (iic->handle->Instance == NULL) ||
        (data == NULL) || (size == 0U)) {
        return DEVICE_ERROR;
    }

    I2C_Regs *i2c = iic->handle->Instance;
    if (wait_idle) {
        Device_Status_e idle_status = IICWaitIdle(i2c);
        if (idle_status != DEVICE_OK) {
            IICRecordStatus(iic);
            return idle_status;
        }
    }

    DL_I2C_startControllerTransferAdvanced(i2c, IICAddress7(iic->dev_address),
        DL_I2C_CONTROLLER_DIRECTION_RX, size, DL_I2C_CONTROLLER_START_ENABLE,
        DL_I2C_CONTROLLER_STOP_ENABLE, DL_I2C_CONTROLLER_ACK_DISABLE);
    delay_cycles(IIC_START_DELAY_CYCLES);

    for (uint16_t i = 0U; i < size; ++i) {
        uint32_t poll = 0U;
        while (DL_I2C_isControllerRXFIFOEmpty(i2c)) {
            if ((DL_I2C_getControllerStatus(i2c) &
                    DL_I2C_CONTROLLER_STATUS_ERROR) != 0U ||
                ++poll >= IIC_POLL_LIMIT) {
                IICRecordStatus(iic);
                DL_I2C_resetControllerTransfer(i2c);
                return (poll >= IIC_POLL_LIMIT) ? DEVICE_TIMEOUT
                                                : DEVICE_ERROR;
            }
        }
        data[i] = DL_I2C_receiveControllerData(i2c);
    }

    Device_Status_e transfer_status = IICWaitTransfer(i2c);
    IICRecordStatus(iic);
    if (transfer_status != DEVICE_OK) {
        DL_I2C_resetControllerTransfer(i2c);
        return transfer_status;
    }

    return DEVICE_OK;
}

IICInstance *IICRegister(IIC_Init_Config_s *conf)
{
    if ((conf == NULL) || (conf->handle == NULL) ||
        (conf->handle->Instance == NULL) ||
        (idx >= MX_IIC_SLAVE_CNT)) {
        return NULL;
    }

    IICInstance *instance =
        (IICInstance *) BSPMalloc(sizeof(IICInstance));
    if (instance == NULL) {
        return NULL;
    }
    memset(instance, 0, sizeof(IICInstance));

    /* DriverLib consumes a 7-bit address. Values above 0x7F are legacy. */
    instance->dev_address =
        (conf->dev_address > 0x7FU) ? (conf->dev_address >> 1U)
                                    : conf->dev_address;
    instance->callback = conf->callback;
    instance->work_mode = conf->work_mode;
    instance->handle = conf->handle;
    instance->id = conf->id;

    iic_instance[idx++] = instance;
    return instance;
}

void IICSetMode(IICInstance *iic, IIC_Work_Mode_e mode)
{
    if (iic != NULL) {
        iic->work_mode = mode;
    }
}

uint32_t IICGetLastControllerStatus(IICInstance *iic)
{
    return (iic != NULL) ? iic->controller_status : 0U;
}

Device_Status_e IICTransmitEx(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e seq_mode)
{
    if (seq_mode != IIC_SEQ_RELEASE && seq_mode != IIC_SEQ_HOLDON) {
        return DEVICE_ERROR;
    }

    return IICBlockingTransmitEx(iic, data, size,
        seq_mode == IIC_SEQ_RELEASE, true);
}

void IICTransmit(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e seq_mode)
{
    (void) IICTransmitEx(iic, data, size, seq_mode);
}

void IICReceive(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e seq_mode)
{
    if (seq_mode != IIC_SEQ_RELEASE && seq_mode != IIC_SEQ_HOLDON) {
        while (1) {
        }
    }

    if (iic == NULL) {
        return;
    }

    iic->rx_buffer = data;
    iic->rx_len = size;

    if (IICBlockingReceiveEx(iic, data, size, true) == DEVICE_OK &&
        iic->callback != NULL) {
        iic->callback(iic);
    }
}

void IICAccessMem(IICInstance *iic, uint16_t mem_addr, uint8_t *data,
    uint16_t size, IIC_Mem_Mode_e mem_mode, uint8_t mem8bit_flag)
{
    uint8_t addr_buf[2];
    uint8_t addr_len = mem8bit_flag ? 1U : 2U;

    if (mem8bit_flag) {
        addr_buf[0] = (uint8_t) mem_addr;
    } else {
        addr_buf[0] = (uint8_t) (mem_addr >> 8U);
        addr_buf[1] = (uint8_t) mem_addr;
    }

    if (mem_mode == IIC_WRITE_MEM) {
        if ((iic == NULL) || (data == NULL) ||
            (size > (uint16_t) (UINT16_MAX - addr_len))) {
            return;
        }

        uint16_t tx_len = (uint16_t) (addr_len + size);
        uint8_t *tx_buf = (uint8_t *) BSPMalloc(tx_len);
        if (tx_buf != NULL) {
            memcpy(tx_buf, addr_buf, addr_len);
            memcpy(&tx_buf[addr_len], data, size);
            (void) IICBlockingTransmitEx(iic, tx_buf, tx_len, true, true);
            BSPFree(tx_buf);
        }
    } else if (mem_mode == IIC_READ_MEM) {
        if (IICBlockingTransmitEx(iic, addr_buf, addr_len, false, true) ==
                DEVICE_OK &&
            IICBlockingReceiveEx(iic, data, size, false) == DEVICE_OK &&
            (iic != NULL) && (iic->callback != NULL)) {
            iic->callback(iic);
        }
    } else {
        while (1) {
        }
    }
}
