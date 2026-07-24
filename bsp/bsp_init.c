#include "bsp_init.h"

#include "bsp_dwt.h"
#include "bsp_encoder.h"

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#endif

void BSPInit(void)
{
    DWT_Init(CPUCLK_FREQ / 1000000U);
    Encoder_BSP_Init();

    NVIC_ClearPendingIRQ(ENCODER_GPIO_GPIOA_INT_IRQN);
    NVIC_ClearPendingIRQ(ENCODER_GPIO_GPIOB_INT_IRQN);
#ifdef USE_FREERTOS
    NVIC_SetPriority(ENCODER_GPIO_GPIOA_INT_IRQN,
        configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(ENCODER_GPIO_GPIOB_INT_IRQN,
        configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
#endif
    NVIC_EnableIRQ(ENCODER_GPIO_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_GPIO_GPIOB_INT_IRQN);
}
