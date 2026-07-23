#ifndef BSP_ENCODER_DECODE_H
#define BSP_ENCODER_DECODE_H

#include <stdint.h>

/**
 * Decode one quadrature state transition.
 *
 * State bit 1 is phase A and bit 0 is phase B. The sequence
 * 00 -> 01 -> 11 -> 10 -> 00 is positive.
 */
int8_t EncoderDecodeTransition(uint8_t previous, uint8_t current);

#endif
