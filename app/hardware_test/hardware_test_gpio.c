#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_GPIO

#include "bsp_gpio.h"

volatile uint32_t hardware_test_gpio_toggle_count;

static GPIOInstance *hardware_test_gpio;

Device_Status_e HardwareTestInit(void)
{
    GPIO_Init_Config_s gpio_config = {
        .GPIOx = GPIO_GRP_0_PORT,
        .GPIO_Pin = GPIO_GRP_0_PIN_0_PIN,
        .pin_state = GPIO_PIN_RESET,
        .exti_mode = GPIO_EXTI_MODE_NONE,
        .gpio_model_callback = NULL,
        .id = NULL,
    };

    hardware_test_gpio = GPIORegister(&gpio_config);
    return (hardware_test_gpio != NULL) ? DEVICE_OK : DEVICE_ERROR;
}

void HardwareTestRun(void)
{
    GPIOToggel(hardware_test_gpio);
    hardware_test_gpio_toggle_count++;
}

#endif
