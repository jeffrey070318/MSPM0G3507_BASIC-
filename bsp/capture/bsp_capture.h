#ifndef BSP_CAPTURE_H
#define BSP_CAPTURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAPTURE_DEVICE_0 = 0,
    CAPTURE_DEVICE_1,
} Capture_Device_e;

void CaptureStart(Capture_Device_e capture);
void CaptureStop(Capture_Device_e capture);
void CaptureReset(Capture_Device_e capture);

/** Read the most recent hardware-captured counter value. */
uint32_t CaptureRead(Capture_Device_e capture);

/** Read the current timer count (for EDGE_COUNT mode, returns accumulated edge count). */
uint32_t CaptureReadCount(Capture_Device_e capture);

/** Convert the most recent captured counter value to microseconds. */
uint32_t CaptureReadTimeUs(Capture_Device_e capture);

/** Return the configured capture timer input clock in Hz. */
uint32_t CaptureGetClockFreq(Capture_Device_e capture);

/** Return the configured capture timer period in ticks. */
uint32_t CaptureGetPeriod(Capture_Device_e capture);

/* ======== Encoder tracking (stateful, call periodically) ======== */
void CaptureEncoderInit(Capture_Device_e capture);
void CaptureEncoderUpdate(Capture_Device_e capture);
int32_t CaptureEncoderGetTotal(Capture_Device_e capture);
int16_t CaptureEncoderGetDelta(Capture_Device_e capture);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CAPTURE_H */