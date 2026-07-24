#ifndef MODULE_INS_H
#define MODULE_INS_H

#include <stdbool.h>
#include <stdint.h>

#define INS_IMU_TOPIC     "imu_data"
#define INS_ENCODER_TOPIC "encoder_data"
#define INS_CMD_TOPIC     "chassis_cmd"

#ifndef INS_WHEEL_RADIUS_M
#define INS_WHEEL_RADIUS_M (0.076f)
#endif
#ifndef INS_ENCODER_PPR
#define INS_ENCODER_PPR (11.0f)
#endif
#ifndef INS_GEAR_RATIO
#define INS_GEAR_RATIO (19.0f)
#endif
#ifndef INS_TARGET_DISTANCE_M
#define INS_TARGET_DISTANCE_M (0.05f)
#endif

#define INS_METERS_PER_COUNT \
    (6.283185307f * INS_WHEEL_RADIUS_M / \
        (INS_ENCODER_PPR * 4.0f * INS_GEAR_RATIO))

typedef struct {
    float x;
    float y;
} INS_Position_t;

typedef struct {
    int32_t left_total;
    int32_t right_total;
} Encoder_Pub_Data_t;

typedef struct {
    float vx;
    float vy;
    float wz;
    bool motion_enabled;
} INS_ChassisCommand_t;

typedef enum {
    INS_STATE_IDLE = 0,
    INS_STATE_RETURNING,
    INS_STATE_DONE,
} INS_State_e;

typedef enum {
    INS_TARGET_FAR = 0,
    INS_TARGET_APPROACHING,
    INS_TARGET_REACHED,
} INS_TargetState_e;

bool INS_Init(void);
void INS_Task(float dt_seconds);
void INS_StartReturn(void);
void INS_ResetOrigin(void);
INS_Position_t INS_GetPosition(void);
INS_State_e INS_GetState(void);
INS_TargetState_e INS_CheckTargetReach(void);

#endif
