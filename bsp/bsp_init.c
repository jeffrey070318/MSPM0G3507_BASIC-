#include "bsp_init.h"

#include "bsp_dwt.h"
#include "bsp_log.h"

void BSPInit(void)
{
    DWT_Init(CPUCLK_FREQ / 1000000U);
    BSPLogInit();
}
