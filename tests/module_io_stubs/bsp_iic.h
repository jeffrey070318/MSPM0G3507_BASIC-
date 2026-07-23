#ifndef TEST_MODULE_IO_BSP_IIC_H
#define TEST_MODULE_IO_BSP_IIC_H

#include <stdint.h>

#include "bsp_def.h"

typedef struct {
    uint32_t marker;
} I2C_HandleTypeDef;

typedef enum {
    IIC_BLOCK_MODE = 0,
    IIC_IT_MODE,
    IIC_DMA_MODE,
} IIC_Work_Mode_e;

typedef enum {
    IIC_READ_MEM = 0,
    IIC_WRITE_MEM,
} IIC_Mem_Mode_e;

typedef struct {
    uint32_t marker;
} IICInstance;

typedef struct {
    I2C_HandleTypeDef *handle;
    uint8_t dev_address;
    IIC_Work_Mode_e work_mode;
    void (*callback)(IICInstance *);
    void *id;
} IIC_Init_Config_s;

extern I2C_HandleTypeDef hi2c2;

#define DL_I2C_CONTROLLER_STATUS_ERROR (1UL << 1U)

IICInstance *IICRegister(IIC_Init_Config_s *config);
void IICAccessMem(IICInstance *iic, uint16_t mem_addr, uint8_t *data,
    uint16_t size, IIC_Mem_Mode_e mode, uint8_t mem8bit_flag);
uint32_t IICGetLastControllerStatus(IICInstance *iic);

#endif
