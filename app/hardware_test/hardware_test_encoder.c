#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_ENCODER

#include "bsp_encoder.h"

volatile int32_t hardware_test_encoder_left_total;
volatile int32_t hardware_test_encoder_right_total;
volatile int16_t hardware_test_encoder_left_speed;
volatile int16_t hardware_test_encoder_right_speed;
volatile uint32_t hardware_test_encoder_left_invalid;
volatile uint32_t hardware_test_encoder_right_invalid;
volatile int32_t hardware_test_encoder_total[2];
volatile int16_t hardware_test_encoder_speed[2];
volatile uint32_t hardware_test_encoder_invalid[2];

Device_Status_e HardwareTestInit(void)
{
    Encoder_Start(&hencoder_left);
    Encoder_Start(&hencoder_right);
    return DEVICE_OK;
}

void HardwareTestRun(void)
{
    Encoder_Update(&hencoder_left);
    Encoder_Update(&hencoder_right);

    hardware_test_encoder_left_total = Encoder_Get_Total(&hencoder_left);
    hardware_test_encoder_right_total = Encoder_Get_Total(&hencoder_right);
    hardware_test_encoder_left_speed = Encoder_Get_Speed(&hencoder_left);
    hardware_test_encoder_right_speed = Encoder_Get_Speed(&hencoder_right);
    hardware_test_encoder_left_invalid =
        Encoder_Get_InvalidTransitions(&hencoder_left);
    hardware_test_encoder_right_invalid =
        Encoder_Get_InvalidTransitions(&hencoder_right);

    hardware_test_encoder_total[0] = hardware_test_encoder_left_total;
    hardware_test_encoder_total[1] = hardware_test_encoder_right_total;
    hardware_test_encoder_speed[0] = hardware_test_encoder_left_speed;
    hardware_test_encoder_speed[1] = hardware_test_encoder_right_speed;
    hardware_test_encoder_invalid[0] = hardware_test_encoder_left_invalid;
    hardware_test_encoder_invalid[1] = hardware_test_encoder_right_invalid;
}

#endif
