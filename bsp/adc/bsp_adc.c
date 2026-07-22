#include "bsp_adc.h"

Device_Status_e ADCRead(uint16_t *value, uint32_t timeout)
{
    if (value == NULL) {
        return DEVICE_ERROR;
    }

#ifndef ADC12_0_INST
    (void) timeout;
    return DEVICE_ERROR;
#else
    ADC12_Regs *adc = ADC12_0_INST;
    uint32_t poll_limit = (timeout == 0U) ? ADC_DEFAULT_TIMEOUT : timeout;

    /* Clear a result flag left by an earlier conversion before triggering. */
    DL_ADC12_clearInterruptStatus(adc, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(adc);

    for (uint32_t poll = 0U; poll < poll_limit; ++poll) {
        if ((DL_ADC12_getRawInterruptStatus(
                 adc, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) != 0U)) {
            *value = DL_ADC12_getMemResult(adc, ADC12_0_ADCMEM_0);
            DL_ADC12_clearInterruptStatus(
                adc, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
            return DEVICE_OK;
        }
    }

    return DEVICE_TIMEOUT;
#endif
}
