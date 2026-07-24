#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "bsp_iic.h"
#include "imu.h"

I2C_HandleTypeDef hi2c2;

static IICInstance iic_instance;
static IIC_Init_Config_s captured_config;

IICInstance *IICRegister(IIC_Init_Config_s *config)
{
    assert(config != NULL);
    captured_config = *config;
    return &iic_instance;
}

void IICAccessMem(IICInstance *iic, uint16_t mem_addr, uint8_t *data,
    uint16_t size, IIC_Mem_Mode_e mode, uint8_t mem8bit_flag)
{
    assert(iic == &iic_instance);
    assert(mode == IIC_READ_MEM);
    assert(mem8bit_flag == 1U);
    memset(data, 0, size);
    if ((mem_addr == JY901S_REG_ANGLE) && (size >= 6U)) {
        data[1] = 0x40U;
        data[3] = 0x20U;
        data[5] = 0xC0U;
    }
}

uint32_t IICGetLastControllerStatus(IICInstance *iic)
{
    assert(iic == &iic_instance);
    return 0U;
}

int main(void)
{
    assert(IMU_Init() == DEVICE_OK);
    assert(captured_config.handle == &hi2c2);
    assert(captured_config.dev_address == JY901S_I2C_ADDR);
    assert(captured_config.work_mode == IIC_BLOCK_MODE);

    float roll;
    float pitch;
    float yaw;
    assert(IMU_ReadAngle(&roll, &pitch, &yaw) == DEVICE_OK);
    assert(roll == 90.0f);
    assert(pitch == 45.0f);
    assert(yaw == -90.0f);
    return 0;
}
