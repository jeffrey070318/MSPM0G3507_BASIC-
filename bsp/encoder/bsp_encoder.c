#include "bsp_encoder.h"

#include <stddef.h>

#include "ti_msp_dl_config.h"

/* ====================== Static state ====================== */
static Encoder_Device_t *g_encoders[ENCODER_MAX_DEVICES];
static uint8_t           g_encoder_count;

/* ====================== Public API ====================== */

void Encoder_Init(Encoder_Device_t *dev)
{
    if ((dev == NULL) || (dev->port_a == NULL) || (dev->port_b == NULL) ||
        (g_encoder_count >= ENCODER_MAX_DEVICES)) {
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

    /* Enable NVIC for the GPIO group. */
    if (dev->port_a == GPIOA) {
        NVIC_EnableIRQ(GPIOA_INT_IRQn);
    } else {
        NVIC_EnableIRQ(GPIOB_INT_IRQn);
    }

    g_encoders[g_encoder_count++] = dev;
}

void Encoder_Update(Encoder_Device_t *dev)
{
    if (dev == NULL) {
        return;
    }

    int32_t total;
    total = dev->total_cnt;

    dev->speed      = (int16_t)(total - dev->last_total);
    dev->last_total = total;
}

int32_t Encoder_Get_Total(Encoder_Device_t *dev)
{
    return (dev != NULL) ? (int32_t)dev->total_cnt : 0;
}

int16_t Encoder_Get_Speed(Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->speed : 0;
}

/* ====================== ISR helpers ====================== */

void Encoder_ISR_ByPortPin(GPIO_TypeDef *port, uint32_t pin)
{
    uint32_t status = DL_GPIO_getEnabledInterruptStatus(port, pin);
    if (status == 0U) {
        return;
    }

    DL_GPIO_clearInterruptStatus(port, pin);

    /* Find the encoder that owns this port+pin (A or B channel). */
    Encoder_Device_t *dev = NULL;
    bool is_pin_a = false;
    for (uint8_t i = 0U; i < g_encoder_count; i++) {
        Encoder_Device_t *d = g_encoders[i];
        if (d == NULL) {
            continue;
        }
        if ((d->port_a == port) && (d->pin_a == pin)) {
            dev = d;
            is_pin_a = true;
            break;
        }
        if ((d->port_b == port) && (d->pin_b == pin)) {
            dev = d;
            is_pin_a = false;
            break;
        }
    }

    if (dev == NULL) {
        return;
    }

    /* Read current levels of A and B. */
    uint32_t a_level =
        DL_GPIO_readPins(dev->port_a, dev->pin_a) ? 1U : 0U;
    uint32_t b_level =
        DL_GPIO_readPins(dev->port_b, dev->pin_b) ? 1U : 0U;

    /*
     * 4x quadrature decode (both channels, both edges):
     *   Interrupt on A:  A^B=1 -> +1,  A^B=0 -> -1
     *   Interrupt on B:  A^B=1 -> -1,  A^B=0 -> +1
     */
    if (a_level ^ b_level) {
        dev->total_cnt += is_pin_a ? 1 : -1;
    } else {
        dev->total_cnt += is_pin_a ? -1 : 1;
    }
}