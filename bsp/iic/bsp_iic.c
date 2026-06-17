#include "bsp_iic.h"

#include "memory.h"
#include "stdlib.h"

static uint8_t idx = 0;
static IICInstance *iic_instance[IIC_DEVICE_CNT] = {NULL};

static uint32_t IICAddress7(uint8_t dev_address)
{
    return (dev_address > 0x7FU) ? ((uint32_t) dev_address >> 1U)
                                 : (uint32_t) dev_address;
}

static void IICWaitIdle(I2C_Regs *i2c)
{
    while ((DL_I2C_getControllerStatus(i2c) &
               (DL_I2C_CONTROLLER_STATUS_BUSY |
                   DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) != 0U) {
    }
}

static HAL_StatusTypeDef IICBlockingTransmit(
    IICInstance *iic, uint8_t *data, uint16_t size)
{
    if ((iic == NULL) || (iic->handle == NULL) || (data == NULL)) {
        return HAL_ERROR;
    }

    I2C_Regs *i2c = iic->handle->Instance;
    IICWaitIdle(i2c);
    DL_I2C_startControllerTransfer(
        i2c, IICAddress7(iic->dev_address), DL_I2C_CONTROLLER_DIRECTION_TX, size);

    for (uint16_t i = 0U; i < size; ++i) {
        while (DL_I2C_isControllerTXFIFOFull(i2c)) {
        }
        DL_I2C_transmitControllerData(i2c, data[i]);
    }

    IICWaitIdle(i2c);
    return ((DL_I2C_getControllerStatus(i2c) & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U)
               ? HAL_ERROR
               : HAL_OK;
}

static HAL_StatusTypeDef IICBlockingReceive(
    IICInstance *iic, uint8_t *data, uint16_t size)
{
    if ((iic == NULL) || (iic->handle == NULL) || (data == NULL)) {
        return HAL_ERROR;
    }

    I2C_Regs *i2c = iic->handle->Instance;
    IICWaitIdle(i2c);
    DL_I2C_startControllerTransfer(
        i2c, IICAddress7(iic->dev_address), DL_I2C_CONTROLLER_DIRECTION_RX, size);

    for (uint16_t i = 0U; i < size; ++i) {
        while (DL_I2C_isControllerRXFIFOEmpty(i2c)) {
        }
        data[i] = DL_I2C_receiveControllerData(i2c);
    }

    IICWaitIdle(i2c);
    return ((DL_I2C_getControllerStatus(i2c) & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U)
               ? HAL_ERROR
               : HAL_OK;
}

IICInstance *IICRegister(IIC_Init_Config_s *conf)
{
    if (idx >= MX_IIC_SLAVE_CNT) {
        while (1) {
        }
    }

    IICInstance *instance = (IICInstance *) malloc(sizeof(IICInstance));
    memset(instance, 0, sizeof(IICInstance));

    instance->dev_address = conf->dev_address << 1;
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

void IICTransmit(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e seq_mode)
{
    if (seq_mode != IIC_SEQ_RELEASE && seq_mode != IIC_SEQ_HOLDON) {
        while (1) {
        }
    }

    (void) IICBlockingTransmit(iic, data, size);
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

    if (IICBlockingReceive(iic, data, size) == HAL_OK && iic->callback != NULL) {
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
        (void) IICBlockingTransmit(iic, addr_buf, addr_len);
        (void) IICBlockingTransmit(iic, data, size);
    } else if (mem_mode == IIC_READ_MEM) {
        (void) IICBlockingTransmit(iic, addr_buf, addr_len);
        (void) IICBlockingReceive(iic, data, size);
        if ((iic != NULL) && (iic->callback != NULL)) {
            iic->callback(iic);
        }
    } else {
        while (1) {
        }
    }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    for (uint8_t i = 0; i < idx; i++) {
        if (iic_instance[i]->handle == hi2c &&
            iic_instance[i]->callback != NULL) {
            iic_instance[i]->callback(iic_instance[i]);
            return;
        }
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    HAL_I2C_MasterRxCpltCallback(hi2c);
}
