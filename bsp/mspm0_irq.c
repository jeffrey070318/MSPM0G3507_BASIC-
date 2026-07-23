#include "bsp_encoder.h"
#include "bsp_usart.h"
#include "ti_msp_dl_config.h"

void UART0_IRQHandler(void)
{
    USARTIRQHandler();
}

/*
 * GROUP1_IRQHandler handles BOTH GPIOA and GPIOB interrupts.
 * GPIOA_INT_IRQn == GPIOB_INT_IRQn == IRQ#1 on MSPM0G3507.
 * GPIOA pins: PA12(L_ENCA), PA13(L_ENCB)
 * GPIOB pins: PB22(R_ENCA), PB23(R_ENCB)
 */
void GROUP1_IRQHandler(void)
{
    /* GPIOA: encoder 0 pins */
    uint32_t status_a = DL_GPIO_getEnabledInterruptStatus(
        GPIOA, DL_GPIO_PIN_12 | DL_GPIO_PIN_13);
    if (status_a & DL_GPIO_PIN_12) {
        Encoder_ISR_ByPortPin(GPIOA, DL_GPIO_PIN_12);
    }
    if (status_a & DL_GPIO_PIN_13) {
        Encoder_ISR_ByPortPin(GPIOA, DL_GPIO_PIN_13);
    }

    /* GPIOB: encoder 1 pins */
    uint32_t status_b = DL_GPIO_getEnabledInterruptStatus(
        GPIOB, DL_GPIO_PIN_22 | DL_GPIO_PIN_23);
    if (status_b & DL_GPIO_PIN_22) {
        Encoder_ISR_ByPortPin(GPIOB, DL_GPIO_PIN_22);
    }
    if (status_b & DL_GPIO_PIN_23) {
        Encoder_ISR_ByPortPin(GPIOB, DL_GPIO_PIN_23);
    }
}