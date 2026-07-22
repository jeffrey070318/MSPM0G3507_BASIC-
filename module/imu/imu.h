#ifndef BSP_IMU_H
#define BSP_IMU_H

#include "bsp_def.h"
#include "bsp_iic.h"

#define JY901S_I2C_ADDR      0x50U

#define JY901S_REG_ACC       0x34U
#define JY901S_REG_GYRO      0x35U
#define JY901S_REG_ANGLE     0x36U
#define JY901S_REG_MAG       0x37U
#define JY901S_REG_BARO      0x38U
#define JY901S_REG_DSTATUS   0x39U
#define JY901S_REG_QUAT      0x3BU

#define JY901S_ACC_LEN       6U
#define JY901S_GYRO_LEN      6U
#define JY901S_ANGLE_LEN     6U
#define JY901S_MAG_LEN       6U
#define JY901S_BARO_LEN      4U

#define JY901S_ACC_SCALE     (16.0f / 32768.0f)
#define JY901S_GYRO_SCALE    (2000.0f / 32768.0f)
#define JY901S_ANGLE_SCALE   (180.0f / 32768.0f)
#define JY901S_G             9.80665f

typedef struct {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t roll, pitch, yaw;
    int16_t mx, my, mz;
} IMU_RawData_t;

typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
    float roll, pitch, yaw;
    float mx, my, mz;
} IMU_Data_t;

typedef struct {
    IICInstance *iic;
} IMU_Handle_t;

Device_Status_e IMU_Init(IMU_Handle_t *imu, IIC_Init_Config_s *conf);
Device_Status_e IMU_ReadRegister(IMU_Handle_t *imu, uint8_t reg, uint8_t *data, uint8_t len);
Device_Status_e IMU_ReadAccel(IMU_Handle_t *imu, float *ax, float *ay, float *az);
Device_Status_e IMU_ReadGyro(IMU_Handle_t *imu, float *gx, float *gy, float *gz);
Device_Status_e IMU_ReadAngle(IMU_Handle_t *imu, float *roll, float *pitch, float *yaw);
Device_Status_e IMU_ReadMag(IMU_Handle_t *imu, float *mx, float *my, float *mz);
Device_Status_e IMU_ReadAll(IMU_Handle_t *imu, IMU_Data_t *data);
Device_Status_e IMU_ReadRaw(IMU_Handle_t *imu, IMU_RawData_t *raw);

#endif