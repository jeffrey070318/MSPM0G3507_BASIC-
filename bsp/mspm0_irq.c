/*
 * MSPM0 interrupt forwarding lives in a maintained BSP source file.
 * SysConfig supplies the IRQ name and peripheral macros; it does not own
 * this implementation and will not overwrite it.
 */
#include "bsp_mspm0_compat.h"

/* The vector entry itself is implemented in bsp_mspm0_compat.c. */
