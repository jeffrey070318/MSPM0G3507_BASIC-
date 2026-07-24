#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_GRAY_SENSOR

#include "gray_sensor.h"

volatile uint32_t hardware_test_gray_last_status = DEVICE_ERROR;
volatile uint32_t hardware_test_gray_update_count;
volatile uint32_t hardware_test_gray_error_count;
volatile uint32_t hardware_test_gray_raw;
volatile uint32_t hardware_test_gray_channel[GRAY_SENSOR_CHANNEL_COUNT];
volatile uint32_t hardware_test_gray_active_count;
volatile float hardware_test_gray_offset;

/* These three values may be modified directly in Live Watch. */
volatile uint32_t hardware_test_gray_active_level = 1U;
volatile uint32_t hardware_test_gray_channel_1_on_right;
volatile uint16_t hardware_test_gray_settle_time_us = 5U;

static GraySensorInstance *hardware_test_gray_sensor;

static GPIO_PinState HardwareTestGrayActiveState(void)
{
    return (hardware_test_gray_active_level != 0U)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;
}

static GraySensor_Channel_Order_e HardwareTestGrayChannelOrder(void)
{
    return (hardware_test_gray_channel_1_on_right != 0U)
        ? GRAY_SENSOR_CHANNEL_1_ON_RIGHT
        : GRAY_SENSOR_CHANNEL_1_ON_LEFT;
}

Device_Status_e HardwareTestInit(void)
{
    GraySensor_Init_Config_s gray_config = {
        .ad0_port = GRAY_SENSOR_GPIO_PORT,
        .ad0_pin = GRAY_SENSOR_GPIO_AD0_PIN,
        .ad1_port = GRAY_SENSOR_GPIO_PORT,
        .ad1_pin = GRAY_SENSOR_GPIO_AD1_PIN,
        .ad2_port = GRAY_SENSOR_GPIO_PORT,
        .ad2_pin = GRAY_SENSOR_GPIO_AD2_PIN,
        .out_port = GRAY_SENSOR_GPIO_PORT,
        .out_pin = GRAY_SENSOR_GPIO_OUT_PIN,
        .active_state = HardwareTestGrayActiveState(),
        .channel_order = HardwareTestGrayChannelOrder(),
        .settle_time_us = hardware_test_gray_settle_time_us,
        .id = NULL,
    };

    hardware_test_gray_sensor = GraySensorRegister(&gray_config);
    return (hardware_test_gray_sensor != NULL) ? DEVICE_OK : DEVICE_ERROR;
}

void HardwareTestRun(void)
{
    if (hardware_test_gray_sensor == NULL) {
        hardware_test_gray_last_status = DEVICE_ERROR;
        hardware_test_gray_error_count++;
        return;
    }

    hardware_test_gray_sensor->active_state =
        HardwareTestGrayActiveState();
    hardware_test_gray_sensor->channel_order =
        HardwareTestGrayChannelOrder();
    hardware_test_gray_sensor->settle_time_us =
        (hardware_test_gray_settle_time_us != 0U)
            ? hardware_test_gray_settle_time_us
            : 1U;

    Device_Status_e status = GraySensorUpdate(hardware_test_gray_sensor);
    hardware_test_gray_last_status = (uint32_t) status;
    if (status != DEVICE_OK) {
        hardware_test_gray_error_count++;
        return;
    }

    hardware_test_gray_raw = hardware_test_gray_sensor->raw_value;
    hardware_test_gray_active_count =
        hardware_test_gray_sensor->active_count;
    hardware_test_gray_offset = hardware_test_gray_sensor->offset;

    for (uint32_t channel = 0U;
         channel < GRAY_SENSOR_CHANNEL_COUNT; channel++) {
        hardware_test_gray_channel[channel] =
            hardware_test_gray_sensor->channel_value[channel];
    }
    hardware_test_gray_update_count++;
}

#endif
