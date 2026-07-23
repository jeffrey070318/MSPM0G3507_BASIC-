#include "bsp_iic.h"
#include "bsp_memory.h"

#include "memory.h"
#include "stdlib.h"

#define IIC_POLL_LIMIT (1000000U)
#define IIC_FIFO_SIZE  (8U)
#define IIC_START_DELAY_CYCLES (32U)
#define IIC_MAX_TRANSFER_SIZE (0x0FFFU)
#define IIC_CONTROLLER_FAILURE_MASK                                      \
    (DL_I2C_CONTROLLER_STATUS_ERROR |                                   \
        DL_I2C_CONTROLLER_STATUS_ARBITRATION_LOST)

static uint8_t idx = 0;
static IICInstance *iic_instance[MX_IIC_SLAVE_CNT] = {NULL};
static I2C_Regs *iic_bus[IIC_DEVICE_CNT] = {NULL};
static volatile uint8_t iic_bus_busy[IIC_DEVICE_CNT];

static uint32_t IICEnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void IICExitCritical(uint32_t primask)
{
    if (primask == 0U) {
        __enable_irq();
    }
}

static int32_t IICReserveBus(I2C_Regs *bus)
{
    int32_t free_index = -1;
    uint32_t primask = IICEnterCritical();
    for (uint8_t i = 0U; i < IIC_DEVICE_CNT; ++i) {
        if (iic_bus[i] == bus) {
            IICExitCritical(primask);
            return (int32_t) i;
        }
        if ((iic_bus[i] == NULL) && (free_index < 0)) {
            free_index = (int32_t) i;
        }
    }
    if (free_index >= 0) {
        iic_bus[free_index] = bus;
    }
    IICExitCritical(primask);
    return free_index;
}

static Device_Status_e IICAcquireBus(IICInstance *iic)
{
    if ((iic == NULL) || (iic->bus_index >= IIC_DEVICE_CNT)) {
        return DEVICE_ERROR;
    }
    if (iic->sequence_held) {
        return DEVICE_OK;
    }

    uint32_t primask = IICEnterCritical();
    if (iic_bus_busy[iic->bus_index] != 0U) {
        IICExitCritical(primask);
        return DEVICE_BUSY;
    }
    iic_bus_busy[iic->bus_index] = 1U;
    IICExitCritical(primask);
    return DEVICE_OK;
}

static void IICReleaseBus(IICInstance *iic)
{
    if ((iic == NULL) || (iic->bus_index >= IIC_DEVICE_CNT)) {
        return;
    }
    uint32_t primask = IICEnterCritical();
    iic->sequence_held = false;
    iic_bus_busy[iic->bus_index] = 0U;
    IICExitCritical(primask);
}

static Device_Status_e IICFinishOperation(
    IICInstance *iic, Device_Status_e status, bool hold_bus)
{
    if ((status == DEVICE_OK) && hold_bus) {
        uint32_t primask = IICEnterCritical();
        iic->sequence_held = true;
        IICExitCritical(primask);
        return status;
    }
    IICReleaseBus(iic);
    return status;
}

static void IICRecordStatus(IICInstance *iic)
{
    if ((iic != NULL) && (iic->handle != NULL) &&
        (iic->handle->Instance != NULL)) {
        iic->controller_status =
            DL_I2C_getControllerStatus(iic->handle->Instance);
    }
}

static Device_Status_e IICWaitIdle(I2C_Regs *i2c)
{
    for (uint32_t poll = 0U; poll < IIC_POLL_LIMIT; ++poll) {
        uint32_t status = DL_I2C_getControllerStatus(i2c);

        if ((status & IIC_CONTROLLER_FAILURE_MASK) != 0U) {
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

        if ((status & IIC_CONTROLLER_FAILURE_MASK) != 0U) {
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
        (data == NULL) || (size == 0U) ||
        (size > IIC_MAX_TRANSFER_SIZE)) {
        return DEVICE_ERROR;
    }

    I2C_Regs *i2c = iic->handle->Instance;
    if (wait_idle) {
        Device_Status_e idle_status = IICWaitIdle(i2c);
        if (idle_status != DEVICE_OK) {
            IICRecordStatus(iic);
            DL_I2C_resetControllerTransfer(i2c);
            return idle_status;
        }
    }

    uint16_t initial_size =
        (size < IIC_FIFO_SIZE) ? size : IIC_FIFO_SIZE;
    DL_I2C_fillControllerTXFIFO(i2c, data, initial_size);

    DL_I2C_startControllerTransferAdvanced(i2c, iic->dev_address,
        DL_I2C_CONTROLLER_DIRECTION_TX, size, DL_I2C_CONTROLLER_START_ENABLE,
        stop_enable ? DL_I2C_CONTROLLER_STOP_ENABLE
                     : DL_I2C_CONTROLLER_STOP_DISABLE,
        DL_I2C_CONTROLLER_ACK_DISABLE);
    delay_cycles(IIC_START_DELAY_CYCLES);

    for (uint16_t i = initial_size; i < size; ++i) {
        uint32_t poll = 0U;
        while (DL_I2C_isControllerTXFIFOFull(i2c)) {
            if ((DL_I2C_getControllerStatus(i2c) &
                    IIC_CONTROLLER_FAILURE_MASK) != 0U ||
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
    IICInstance *iic, uint8_t *data, uint16_t size, bool stop_enable,
    bool wait_idle)
{
    if ((iic == NULL) || (iic->handle == NULL) ||
        (iic->handle->Instance == NULL) ||
        (data == NULL) || (size == 0U) ||
        (size > IIC_MAX_TRANSFER_SIZE)) {
        return DEVICE_ERROR;
    }

    I2C_Regs *i2c = iic->handle->Instance;
    if (wait_idle) {
        Device_Status_e idle_status = IICWaitIdle(i2c);
        if (idle_status != DEVICE_OK) {
            IICRecordStatus(iic);
            DL_I2C_resetControllerTransfer(i2c);
            return idle_status;
        }
    }

    DL_I2C_startControllerTransferAdvanced(i2c, iic->dev_address,
        DL_I2C_CONTROLLER_DIRECTION_RX, size, DL_I2C_CONTROLLER_START_ENABLE,
        stop_enable ? DL_I2C_CONTROLLER_STOP_ENABLE
                    : DL_I2C_CONTROLLER_STOP_DISABLE,
        DL_I2C_CONTROLLER_ACK_DISABLE);
    delay_cycles(IIC_START_DELAY_CYCLES);

    for (uint16_t i = 0U; i < size; ++i) {
        uint32_t poll = 0U;
        while (DL_I2C_isControllerRXFIFOEmpty(i2c)) {
            if ((DL_I2C_getControllerStatus(i2c) &
                    IIC_CONTROLLER_FAILURE_MASK) != 0U ||
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
        (conf->dev_address > 0x7FU) ||
        (conf->work_mode > IIC_DMA_MODE) ||
        (idx >= MX_IIC_SLAVE_CNT)) {
        return NULL;
    }

    IICInstance *instance =
        (IICInstance *) BSPMalloc(sizeof(IICInstance));
    if (instance == NULL) {
        return NULL;
    }
    memset(instance, 0, sizeof(IICInstance));

    int32_t bus_index = IICReserveBus(conf->handle->Instance);
    if (bus_index < 0) {
        BSPFree(instance);
        return NULL;
    }

    instance->dev_address = conf->dev_address;
    instance->callback = conf->callback;
    instance->work_mode = conf->work_mode;
    instance->handle = conf->handle;
    instance->id = conf->id;
    instance->bus_index = (uint8_t) bus_index;

    iic_instance[idx++] = instance;
    return instance;
}

void IICSetMode(IICInstance *iic, IIC_Work_Mode_e mode)
{
    if ((iic != NULL) && (mode <= IIC_DMA_MODE)) {
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

    Device_Status_e status = IICAcquireBus(iic);
    if (status != DEVICE_OK) {
        return status;
    }
    status = IICBlockingTransmitEx(iic, data, size,
        seq_mode == IIC_SEQ_RELEASE, true);
    return IICFinishOperation(iic, status, seq_mode == IIC_SEQ_HOLDON);
}

void IICTransmit(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e seq_mode)
{
    (void) IICTransmitEx(iic, data, size, seq_mode);
}

void IICReceive(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e seq_mode)
{
    (void) IICReceiveEx(iic, data, size, seq_mode);
}

Device_Status_e IICReceiveEx(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e seq_mode)
{
    if (seq_mode != IIC_SEQ_RELEASE && seq_mode != IIC_SEQ_HOLDON) {
        return DEVICE_ERROR;
    }

    if (iic == NULL) {
        return DEVICE_ERROR;
    }

    iic->rx_buffer = data;
    iic->rx_len = size;

    Device_Status_e status = IICAcquireBus(iic);
    if (status != DEVICE_OK) {
        return status;
    }
    status = IICBlockingReceiveEx(iic, data, size,
        seq_mode == IIC_SEQ_RELEASE, true);
    status = IICFinishOperation(iic, status, seq_mode == IIC_SEQ_HOLDON);
    if ((status == DEVICE_OK) && (iic->callback != NULL)) {
        iic->callback(iic);
    }
    return status;
}

void IICAccessMem(IICInstance *iic, uint16_t mem_addr, uint8_t *data,
    uint16_t size, IIC_Mem_Mode_e mem_mode, uint8_t mem8bit_flag)
{
    (void) IICAccessMemEx(
        iic, mem_addr, data, size, mem_mode, mem8bit_flag);
}

Device_Status_e IICAccessMemEx(IICInstance *iic, uint16_t mem_addr,
    uint8_t *data, uint16_t size, IIC_Mem_Mode_e mem_mode,
    uint8_t mem8bit_flag)
{
    uint8_t addr_buf[2];
    uint8_t addr_len = mem8bit_flag ? 1U : 2U;

    if ((iic == NULL) || (data == NULL) || (size == 0U) ||
        (size > IIC_MAX_TRANSFER_SIZE) ||
        ((mem_mode != IIC_WRITE_MEM) && (mem_mode != IIC_READ_MEM))) {
        return DEVICE_ERROR;
    }

    Device_Status_e acquire_status = IICAcquireBus(iic);
    if (acquire_status != DEVICE_OK) {
        return acquire_status;
    }

    if (mem8bit_flag) {
        addr_buf[0] = (uint8_t) mem_addr;
    } else {
        addr_buf[0] = (uint8_t) (mem_addr >> 8U);
        addr_buf[1] = (uint8_t) mem_addr;
    }

    if (mem_mode == IIC_WRITE_MEM) {
        if (size > (uint16_t) (IIC_MAX_TRANSFER_SIZE - addr_len)) {
            IICReleaseBus(iic);
            return DEVICE_ERROR;
        }

        uint16_t tx_len = (uint16_t) (addr_len + size);
        uint8_t *tx_buf = (uint8_t *) BSPMalloc(tx_len);
        if (tx_buf == NULL) {
            IICReleaseBus(iic);
            return DEVICE_ERROR;
        }
        memcpy(tx_buf, addr_buf, addr_len);
        memcpy(&tx_buf[addr_len], data, size);
        Device_Status_e status =
            IICBlockingTransmitEx(iic, tx_buf, tx_len, true, true);
        BSPFree(tx_buf);
        return IICFinishOperation(iic, status, false);
    }

    iic->rx_buffer = data;
    iic->rx_len = size;
    Device_Status_e status =
        IICBlockingTransmitEx(iic, addr_buf, addr_len, false, true);
    if (status == DEVICE_OK) {
        status = IICBlockingReceiveEx(iic, data, size, true, false);
    }
    status = IICFinishOperation(iic, status, false);
    if ((status == DEVICE_OK) && (iic->callback != NULL)) {
        iic->callback(iic);
    }
    return status;
}

void IICAbortSequence(IICInstance *iic)
{
    if ((iic == NULL) || (iic->handle == NULL) ||
        (iic->handle->Instance == NULL)) {
        return;
    }
    IICRecordStatus(iic);
    DL_I2C_resetControllerTransfer(iic->handle->Instance);
    IICReleaseBus(iic);
}
