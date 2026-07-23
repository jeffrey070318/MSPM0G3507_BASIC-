#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "bsp_iic.h"

uint32_t DL_I2C_getControllerStatus(I2C_Regs *i2c)
{
    return i2c->status;
}

void DL_I2C_fillControllerTXFIFO(
    I2C_Regs *i2c, const uint8_t *data, uint16_t size)
{
    (void) i2c;
    (void) data;
    (void) size;
}

void DL_I2C_startControllerTransferAdvanced(I2C_Regs *i2c,
    uint32_t target_address, uint32_t direction, uint16_t size,
    uint32_t start, uint32_t stop, uint32_t ack)
{
    (void) start;
    (void) ack;
    i2c->target_address = target_address;
    i2c->direction = direction;
    i2c->transfer_size = size;
    i2c->stop_mode = stop;
    i2c->status = DL_I2C_CONTROLLER_STATUS_IDLE;
}

bool DL_I2C_isControllerTXFIFOFull(I2C_Regs *i2c)
{
    (void) i2c;
    return false;
}

void DL_I2C_transmitControllerData(I2C_Regs *i2c, uint8_t data)
{
    (void) i2c;
    (void) data;
}

bool DL_I2C_isControllerRXFIFOEmpty(I2C_Regs *i2c)
{
    (void) i2c;
    return false;
}

uint8_t DL_I2C_receiveControllerData(I2C_Regs *i2c)
{
    return i2c->rx_value;
}

void DL_I2C_resetControllerTransfer(I2C_Regs *i2c)
{
    i2c->reset_count++;
}

int main(void)
{
    I2C_Regs regs = {
        .status = DL_I2C_CONTROLLER_STATUS_IDLE,
        .rx_value = 0x5AU,
    };
    I2C_HandleTypeDef handle = {.Instance = &regs};
    IIC_Init_Config_s config = {
        .handle = &handle,
        .dev_address = 0x3CU,
        .work_mode = IIC_BLOCK_MODE,
    };
    IICInstance *instance = IICRegister(&config);
    assert(instance != NULL);
    assert(instance->dev_address == 0x3CU);

    IIC_Init_Config_s legacy_address = config;
    legacy_address.dev_address = 0xD0U;
    assert(IICRegister(&legacy_address) == NULL);

    uint8_t value = 0U;
    regs.status = DL_I2C_CONTROLLER_STATUS_IDLE;
    assert(IICReceiveEx(instance, &value, 1U, IIC_SEQ_HOLDON) == DEVICE_OK);
    assert(value == 0x5AU);
    assert(regs.stop_mode == DL_I2C_CONTROLLER_STOP_DISABLE);

    assert(IICReceiveEx(instance, &value, 1U,
               (IIC_Seq_Mode_e) 99) == DEVICE_ERROR);
    assert(IICTransmitEx(instance, &value, 4096U,
               IIC_SEQ_RELEASE) == DEVICE_ERROR);
    assert(IICAccessMemEx(instance, 0U, &value, 1U,
               (IIC_Mem_Mode_e) 99, 1U) == DEVICE_ERROR);

    IIC_Init_Config_s second_config = config;
    second_config.dev_address = 0x3DU;
    IICInstance *second = IICRegister(&second_config);
    assert(second != NULL);
    assert(IICTransmitEx(instance, &value, 1U, IIC_SEQ_HOLDON) == DEVICE_OK);
    assert(IICTransmitEx(second, &value, 1U, IIC_SEQ_RELEASE) == DEVICE_BUSY);
    assert(IICTransmitEx(instance, &value, 1U, IIC_SEQ_RELEASE) == DEVICE_OK);
    assert(IICTransmitEx(second, &value, 1U, IIC_SEQ_RELEASE) == DEVICE_OK);

    assert(IICTransmitEx(instance, &value, 1U, IIC_SEQ_HOLDON) == DEVICE_OK);
    IICAbortSequence(instance);
    assert(regs.reset_count != 0U);
    assert(IICTransmitEx(second, &value, 1U, IIC_SEQ_RELEASE) == DEVICE_OK);

    return 0;
}
