#ifndef HARDWARE_TEST_TASK_H
#define HARDWARE_TEST_TASK_H

#include "hardware_test_config.h"

#if HARDWARE_TEST_MODE != HARDWARE_TEST_NONE
#include "bsp_def.h"

Device_Status_e HardwareTestInit(void);
void HardwareTestRun(void);
void StartHARDWARETESTTASK(void *argument);
#endif

#endif
