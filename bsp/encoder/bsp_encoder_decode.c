#include "bsp_encoder_decode.h"

int8_t EncoderDecodeTransition(uint8_t previous, uint8_t current)
{
    static const int8_t transition_table[16] = {
         0,  1, -1,  0,
        -1,  0,  0,  1,
         1,  0,  0, -1,
         0, -1,  1,  0,
    };

    uint8_t index = (uint8_t) (((previous & 0x03U) << 2U) |
                               (current & 0x03U));
    return transition_table[index];
}
