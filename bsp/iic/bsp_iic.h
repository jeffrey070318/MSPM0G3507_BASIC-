#ifndef BSP_IIC_H
#define BSP_IIC_H

#include "bsp_device.h"

#define IIC_DEVICE_CNT 2U
#define MX_IIC_SLAVE_CNT 8U

typedef enum {
    IIC_BLOCK_MODE = 0,
    IIC_IT_MODE,
    IIC_DMA_MODE,
} IIC_Work_Mode_e;

typedef enum {
    IIC_READ_MEM = 0,
    IIC_WRITE_MEM,
} IIC_Mem_Mode_e;

typedef enum {
    IIC_SEQ_RELEASE = 0,
    IIC_SEQ_HOLDON,
} IIC_Seq_Mode_e;

typedef struct iic_temp_s {
    I2C_HandleTypeDef *handle;
    uint8_t dev_address;
    IIC_Work_Mode_e work_mode;
    uint8_t *rx_buffer;
    uint16_t rx_len;
    void (*callback)(struct iic_temp_s *);
    void *id;
    volatile uint32_t controller_status;
    uint8_t bus_index;
    bool sequence_held;
} IICInstance;

typedef struct {
    I2C_HandleTypeDef *handle;
    uint8_t dev_address;
    IIC_Work_Mode_e work_mode;
    void (*callback)(IICInstance *);
    void *id;
} IIC_Init_Config_s;

IICInstance *IICRegister(IIC_Init_Config_s *conf);
void IICSetMode(IICInstance *iic, IIC_Work_Mode_e mode);
uint32_t IICGetLastControllerStatus(IICInstance *iic);
Device_Status_e IICTransmitEx(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e mode);
Device_Status_e IICReceiveEx(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e mode);
void IICTransmit(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e mode);
void IICReceive(
    IICInstance *iic, uint8_t *data, uint16_t size, IIC_Seq_Mode_e mode);
void IICAccessMem(IICInstance *iic, uint16_t mem_addr, uint8_t *data,
    uint16_t size, IIC_Mem_Mode_e mode, uint8_t mem8bit_flag);
Device_Status_e IICAccessMemEx(IICInstance *iic, uint16_t mem_addr,
    uint8_t *data, uint16_t size, IIC_Mem_Mode_e mode,
    uint8_t mem8bit_flag);
void IICAbortSequence(IICInstance *iic);

#endif
