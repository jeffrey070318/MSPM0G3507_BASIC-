#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "bsp_def.h"

#define ADC_DEFAULT_TIMEOUT (1000000U)

Device_Status_e ADCRead(uint16_t *value, uint32_t timeout);

#endif
