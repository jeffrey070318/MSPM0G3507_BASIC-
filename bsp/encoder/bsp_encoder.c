#include "bsp_encoder.h"

#include <limits.h>

#include "bsp_encoder_decode.h"

Encoder_Device_t hencoder_left = {
    .phase_a_port = ENCODER_GPIO_ENC_L_A_PORT,
    .phase_a_pin = ENCODER_GPIO_ENC_L_A_PIN,
    .phase_b_port = ENCODER_GPIO_ENC_L_B_PORT,
    .phase_b_pin = ENCODER_GPIO_ENC_L_B_PIN,
    .reverse = false,
};

Encoder_Device_t hencoder_right = {
    .phase_a_port = ENCODER_GPIO_ENC_R_A_PORT,
    .phase_a_pin = ENCODER_GPIO_ENC_R_A_PIN,
    .phase_b_port = ENCODER_GPIO_ENC_R_B_PORT,
    .phase_b_pin = ENCODER_GPIO_ENC_R_B_PIN,
    .reverse = false,
};

static uint8_t EncoderReadState(const Encoder_Device_t *dev)
{
    uint8_t phase_a =
        (DL_GPIO_readPins(dev->phase_a_port, dev->phase_a_pin) != 0U) ? 1U : 0U;
    uint8_t phase_b =
        (DL_GPIO_readPins(dev->phase_b_port, dev->phase_b_pin) != 0U) ? 1U : 0U;
    return (uint8_t) ((phase_a << 1U) | phase_b);
}

static int16_t EncoderClampSpeed(int64_t delta)
{
    if (delta > INT16_MAX) {
        return INT16_MAX;
    }
    if (delta < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t) delta;
}

static void EncoderAccumulate(Encoder_Device_t *dev, int8_t delta)
{
    if ((delta > 0) && (dev->total_cnt < INT32_MAX)) {
        dev->total_cnt++;
    } else if ((delta < 0) && (dev->total_cnt > INT32_MIN)) {
        dev->total_cnt--;
    }
}

void Encoder_BSP_Init(void)
{
    Encoder_Start(&hencoder_left);
    Encoder_Start(&hencoder_right);
}

void Encoder_Start(Encoder_Device_t *dev)
{
    if ((dev == NULL) || (dev->phase_a_port == NULL) ||
        (dev->phase_b_port == NULL) || (dev->phase_a_pin == 0U) ||
        (dev->phase_b_pin == 0U)) {
        return;
    }

    dev->total_cnt  = 0;
    dev->speed      = 0;
    dev->last_total = 0;

    /* Determine IOMUX PINCM indices.
     * Encoder 0 (Left):  PA12=PINCM34, PA13=PINCM35
     * Encoder 1 (Right): PB22=PINCM50, PB23=PINCM51 */
    uint32_t idx_a;
    uint32_t idx_b;

    if (dev->port_a == GPIOA) {
        idx_a = (uint32_t)IOMUX_PINCM34;
        idx_b = (uint32_t)IOMUX_PINCM35;
    } else {
        idx_a = (uint32_t)IOMUX_PINCM50;
        idx_b = (uint32_t)IOMUX_PINCM51;
    }

    /* Configure A as GPIO input with interrupt on both edges.
     * PF=1 (GPIO), INENA=1, PC_CONNECTED=1, EDGE_SEL=3 (both), IEN=1 */
    IOMUX->SECCFG.PINCM[idx_a] =
        IOMUX_PINCM_INENA_ENABLE | IOMUX_PINCM_PC_CONNECTED |
        ((uint32_t)0x00000001) |             /* PF = GPIO */
        ((uint32_t)0x3U << 18U) |            /* EDGE_SEL = both edges */
        ((uint32_t)0x1U << 20U);             /* IEN = interrupt enable */

    /* Configure B as GPIO input with interrupt on both edges. */
    IOMUX->SECCFG.PINCM[idx_b] =
        IOMUX_PINCM_INENA_ENABLE | IOMUX_PINCM_PC_CONNECTED |
        ((uint32_t)0x00000001) |             /* PF = GPIO */
        ((uint32_t)0x3U << 18U) |            /* EDGE_SEL = both edges */
        ((uint32_t)0x1U << 20U);             /* IEN = interrupt enable */

    /* Clear any pending interrupts on both pins. */
    DL_GPIO_clearInterruptStatus(dev->port_a, dev->pin_a);
    DL_GPIO_clearInterruptStatus(dev->port_b, dev->pin_b);

    /* Enable GPIO interrupts on both pins. */
    DL_GPIO_enableInterrupt(dev->port_a, dev->pin_a);
    DL_GPIO_enableInterrupt(dev->port_b, dev->pin_b);

    /* GPIOA_INT_IRQn == GPIOB_INT_IRQn == GROUP1 on MSPM0G3507.
     * Enabling either is equivalent; both ports share GROUP1_IRQHandler. */
    NVIC_EnableIRQ(GPIOA_INT_IRQn);

    g_encoders[g_encoder_count++] = dev;
}

void Encoder_Update(Encoder_Device_t *dev)
{
    if ((dev == NULL) || !dev->started) {
        return;
    }

    int32_t current = dev->total_cnt;
    int64_t delta = (int64_t) current - (int64_t) dev->last_sample_cnt;
    dev->speed = EncoderClampSpeed(delta);
    dev->last_sample_cnt = current;
}

void Encoder_SetReverse(Encoder_Device_t *dev, bool reverse)
{
    if (dev != NULL) {
        dev->reverse = reverse;
    }
}

int32_t Encoder_Get_Total(const Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->total_cnt : 0;
}

int16_t Encoder_Get_Speed(const Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->speed : 0;
}

uint32_t Encoder_Get_InvalidTransitions(const Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->invalid_transition_count : 0U;
}
