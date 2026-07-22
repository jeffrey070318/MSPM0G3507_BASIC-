#include "imu.h"

static int16_t IMU_CombineI16(uint8_t lo, uint8_t hi)
{
    return (int16_t)(((uint16_t)hi << 8U) | (uint16_t)lo);
}

static void IMU_ParseAccel(const uint8_t buf[6], float *ax, float *ay, float *az)
{
    if (ax) *ax = (float)IMU_CombineI16(buf[0], buf[1]) * JY901S_ACC_SCALE * JY901S_G;
    if (ay) *ay = (float)IMU_CombineI16(buf[2], buf[3]) * JY901S_ACC_SCALE * JY901S_G;
    if (az) *az = (float)IMU_CombineI16(buf[4], buf[5]) * JY901S_ACC_SCALE * JY901S_G;
}

static void IMU_ParseGyro(const uint8_t buf[6], float *gx, float *gy, float *gz)
{
    if (gx) *gx = (float)IMU_CombineI16(buf[0], buf[1]) * JY901S_GYRO_SCALE;
    if (gy) *gy = (float)IMU_CombineI16(buf[2], buf[3]) * JY901S_GYRO_SCALE;
    if (gz) *gz = (float)IMU_CombineI16(buf[4], buf[5]) * JY901S_GYRO_SCALE;
}

static void IMU_ParseAngle(const uint8_t buf[6], float *roll, float *pitch, float *yaw)
{
    if (roll)  *roll  = (float)IMU_CombineI16(buf[0], buf[1]) * JY901S_ANGLE_SCALE;
    if (pitch) *pitch = (float)IMU_CombineI16(buf[2], buf[3]) * JY901S_ANGLE_SCALE;
    if (yaw)   *yaw   = (float)IMU_CombineI16(buf[4], buf[5]) * JY901S_ANGLE_SCALE;
}

static void IMU_ParseMag(const uint8_t buf[6], float *mx, float *my, float *mz)
{
    if (mx) *mx = (float)IMU_CombineI16(buf[0], buf[1]);
    if (my) *my = (float)IMU_CombineI16(buf[2], buf[3]);
    if (mz) *mz = (float)IMU_CombineI16(buf[4], buf[5]);
}

Device_Status_e IMU_Init(IMU_Handle_t *imu, IIC_Init_Config_s *conf)
{
    if ((imu == NULL) || (conf == NULL)) return DEVICE_ERROR;
    imu->iic = IICRegister(conf);
    return (imu->iic != NULL) ? DEVICE_OK : DEVICE_ERROR;
}

Device_Status_e IMU_ReadRegister(IMU_Handle_t *imu, uint8_t reg, uint8_t *data, uint8_t len)
{
    if ((imu == NULL) || (imu->iic == NULL) || (data == NULL) || (len == 0U))
        return DEVICE_ERROR;

    IICAccessMem(imu->iic, (uint16_t)reg, data, (uint16_t)len, IIC_READ_MEM, 1U);

    uint32_t status = IICGetLastControllerStatus(imu->iic);
    return ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) ? DEVICE_ERROR : DEVICE_OK;
}

Device_Status_e IMU_ReadAccel(IMU_Handle_t *imu, float *ax, float *ay, float *az)
{
    uint8_t buf[JY901S_ACC_LEN];
    Device_Status_e ret = IMU_ReadRegister(imu, JY901S_REG_ACC, buf, JY901S_ACC_LEN);
    if (ret != DEVICE_OK) return ret;
    IMU_ParseAccel(buf, ax, ay, az);
    return DEVICE_OK;
}

Device_Status_e IMU_ReadGyro(IMU_Handle_t *imu, float *gx, float *gy, float *gz)
{
    uint8_t buf[JY901S_GYRO_LEN];
    Device_Status_e ret = IMU_ReadRegister(imu, JY901S_REG_GYRO, buf, JY901S_GYRO_LEN);
    if (ret != DEVICE_OK) return ret;
    IMU_ParseGyro(buf, gx, gy, gz);
    return DEVICE_OK;
}

Device_Status_e IMU_ReadAngle(IMU_Handle_t *imu, float *roll, float *pitch, float *yaw)
{
    uint8_t buf[JY901S_ANGLE_LEN];
    Device_Status_e ret = IMU_ReadRegister(imu, JY901S_REG_ANGLE, buf, JY901S_ANGLE_LEN);
    if (ret != DEVICE_OK) return ret;
    IMU_ParseAngle(buf, roll, pitch, yaw);
    return DEVICE_OK;
}

Device_Status_e IMU_ReadMag(IMU_Handle_t *imu, float *mx, float *my, float *mz)
{
    uint8_t buf[JY901S_MAG_LEN];
    Device_Status_e ret = IMU_ReadRegister(imu, JY901S_REG_MAG, buf, JY901S_MAG_LEN);
    if (ret != DEVICE_OK) return ret;
    IMU_ParseMag(buf, mx, my, mz);
    return DEVICE_OK;
}

Device_Status_e IMU_ReadAll(IMU_Handle_t *imu, IMU_Data_t *data)
{
    if ((imu == NULL) || (data == NULL)) return DEVICE_ERROR;

    Device_Status_e ret;
    uint8_t buf[6];

    ret = IMU_ReadRegister(imu, JY901S_REG_ACC, buf, JY901S_ACC_LEN);
    if (ret != DEVICE_OK) return ret;
    IMU_ParseAccel(buf, &data->ax, &data->ay, &data->az);

    ret = IMU_ReadRegister(imu, JY901S_REG_GYRO, buf, JY901S_GYRO_LEN);
    if (ret != DEVICE_OK) return ret;
    IMU_ParseGyro(buf, &data->gx, &data->gy, &data->gz);

    ret = IMU_ReadRegister(imu, JY901S_REG_ANGLE, buf, JY901S_ANGLE_LEN);
    if (ret != DEVICE_OK) return ret;
    IMU_ParseAngle(buf, &data->roll, &data->pitch, &data->yaw);

    ret = IMU_ReadRegister(imu, JY901S_REG_MAG, buf, JY901S_MAG_LEN);
    if (ret != DEVICE_OK) return ret;
    IMU_ParseMag(buf, &data->mx, &data->my, &data->mz);

    return DEVICE_OK;
}

Device_Status_e IMU_ReadRaw(IMU_Handle_t *imu, IMU_RawData_t *raw)
{
    if ((imu == NULL) || (raw == NULL)) return DEVICE_ERROR;

    Device_Status_e ret;
    uint8_t buf[6];

    ret = IMU_ReadRegister(imu, JY901S_REG_ACC, buf, JY901S_ACC_LEN);
    if (ret != DEVICE_OK) return ret;
    raw->ax = IMU_CombineI16(buf[0], buf[1]);
    raw->ay = IMU_CombineI16(buf[2], buf[3]);
    raw->az = IMU_CombineI16(buf[4], buf[5]);

    ret = IMU_ReadRegister(imu, JY901S_REG_GYRO, buf, JY901S_GYRO_LEN);
    if (ret != DEVICE_OK) return ret;
    raw->gx = IMU_CombineI16(buf[0], buf[1]);
    raw->gy = IMU_CombineI16(buf[2], buf[3]);
    raw->gz = IMU_CombineI16(buf[4], buf[5]);

    ret = IMU_ReadRegister(imu, JY901S_REG_ANGLE, buf, JY901S_ANGLE_LEN);
    if (ret != DEVICE_OK) return ret;
    raw->roll  = IMU_CombineI16(buf[0], buf[1]);
    raw->pitch = IMU_CombineI16(buf[2], buf[3]);
    raw->yaw   = IMU_CombineI16(buf[4], buf[5]);

    ret = IMU_ReadRegister(imu, JY901S_REG_MAG, buf, JY901S_MAG_LEN);
    if (ret != DEVICE_OK) return ret;
    raw->mx = IMU_CombineI16(buf[0], buf[1]);
    raw->my = IMU_CombineI16(buf[2], buf[3]);
    raw->mz = IMU_CombineI16(buf[4], buf[5]);

    return DEVICE_OK;
}