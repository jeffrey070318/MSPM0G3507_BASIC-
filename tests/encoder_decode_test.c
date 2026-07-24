#include <stdint.h>
#include <stdio.h>

#include "bsp_encoder_decode.h"

static int failures;

static void ExpectDelta(uint8_t previous, uint8_t current, int8_t expected)
{
    int8_t actual = EncoderDecodeTransition(previous, current);
    if (actual != expected) {
        printf("transition %u -> %u: expected %d, got %d\n",
            previous, current, expected, actual);
        failures++;
    }
}

int main(void)
{
    static const int8_t expected[16] = {
         0,  1, -1,  0,
        -1,  0,  0,  1,
         1,  0,  0, -1,
         0, -1,  1,  0,
    };

    for (uint8_t previous = 0U; previous < 4U; ++previous) {
        for (uint8_t current = 0U; current < 4U; ++current) {
            ExpectDelta(previous, current,
                expected[(previous << 2U) | current]);
        }
    }

    const uint8_t forward[] = {0U, 1U, 3U, 2U, 0U};
    const uint8_t reverse[] = {0U, 2U, 3U, 1U, 0U};
    int forward_sum = 0;
    int reverse_sum = 0;
    for (uint8_t i = 1U; i < 5U; ++i) {
        forward_sum += EncoderDecodeTransition(forward[i - 1U], forward[i]);
        reverse_sum += EncoderDecodeTransition(reverse[i - 1U], reverse[i]);
    }

    if (forward_sum != 4) {
        printf("forward cycle: expected 4, got %d\n", forward_sum);
        failures++;
    }
    if (reverse_sum != -4) {
        printf("reverse cycle: expected -4, got %d\n", reverse_sum);
        failures++;
    }

    return (failures == 0) ? 0 : 1;
}
