#include "bsp_usart.h"
#include "bsp_encoder.h"
#include "bsp_gpio.h"

void UART1_INST_IRQHandler(void);
void UART2_INST_IRQHandler(void);
void UART3_INST_IRQHandler(void);
void GROUP1_IRQHandler(void);

void UART1_INST_IRQHandler(void)
{
    USARTIRQHandlerFor(UART1_INST);
}

void UART2_INST_IRQHandler(void)
{
    USARTIRQHandlerFor(UART2_INST);
}

void UART3_INST_IRQHandler(void)
{
    USARTIRQHandlerFor(UART3_INST);
}

static void GPIOAInterruptHandler(void)
{
    for (;;) {
        uint32_t pending = (uint32_t) DL_GPIO_getPendingInterrupt(GPIOA);
        switch (pending) {
        case ENCODER_GPIO_ENC_L_A_IIDX:
            Encoder_OnEdge(&hencoder_left);
            break;
        case ENCODER_GPIO_ENC_L_B_IIDX:
            Encoder_OnEdge(&hencoder_left);
            break;
        default: {
            uint32_t pin = GPIOPinFromInterruptIndex(pending);
            if (pin == 0U) {
                return;
            }
            GPIOInterruptCallbackForPort(GPIOA, pin);
            break;
        }
        }
    }
}

static void GPIOBInterruptHandler(void)
{
    for (;;) {
        uint32_t pending = (uint32_t) DL_GPIO_getPendingInterrupt(GPIOB);
        switch (pending) {
        case ENCODER_GPIO_ENC_R_A_IIDX:
            Encoder_OnEdge(&hencoder_right);
            break;
        case ENCODER_GPIO_ENC_R_B_IIDX:
            Encoder_OnEdge(&hencoder_right);
            break;
        default: {
            uint32_t pin = GPIOPinFromInterruptIndex(pending);
            if (pin == 0U) {
                return;
            }
            GPIOInterruptCallbackForPort(GPIOB, pin);
            break;
        }
        }
    }
}

void GROUP1_IRQHandler(void)
{
    for (;;) {
        switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        case ENCODER_GPIO_GPIOA_INT_IIDX:
            GPIOAInterruptHandler();
            break;
        case ENCODER_GPIO_GPIOB_INT_IIDX:
            GPIOBInterruptHandler();
            break;
        default:
            return;
        }
    }
}
