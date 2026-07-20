#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_OLED

#include "oled.h"

volatile uint32_t hardware_test_oled_refresh_count;
volatile uint32_t hardware_test_iic_controller_status;
volatile uint8_t hardware_test_iic_address;

Device_Status_e HardwareTestInit(void)
{
    Device_Status_e status = OLED_init_ex();

    hardware_test_iic_controller_status = OLED_get_iic_status();
    hardware_test_iic_address = OLED_get_iic_address();
    if (status != DEVICE_OK) {
        return status;
    }

    OLED_operate_gram(PEN_CLEAR);
    OLED_printf(0U, 0U, "MSPM0 READY");
    OLED_printf(1U, 0U, "I2C OLED OK");
    OLED_refresh_gram();
    hardware_test_oled_refresh_count++;
    return DEVICE_OK;
}

void HardwareTestRun(void)
{
    hardware_test_iic_controller_status = OLED_get_iic_status();
    hardware_test_iic_address = OLED_get_iic_address();
}

#endif
