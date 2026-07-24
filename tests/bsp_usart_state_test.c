#include <assert.h>
#include <stdint.h>

#include "bsp_usart_state.h"

int main(void)
{
    USART_Tx_Completion_State_s state = {0};

    assert(!USARTTxCompletionOnEOT(&state));

    USARTTxCompletionBegin(&state);
    assert(!USARTTxCompletionOnEOT(&state));
    assert(USARTTxCompletionOnDataLoaded(&state));

    USARTTxCompletionBegin(&state);
    assert(!USARTTxCompletionOnDataLoaded(&state));
    assert(USARTTxCompletionOnEOT(&state));

    USARTTxCompletionReset(&state);
    assert(!state.active);
    assert(!state.data_loaded);
    assert(!state.eot_seen);

    uint8_t storage[5] = {0};
    USART_Rx_Ring_State_s ring = {0};
    USARTRxRingInit(&ring, storage, sizeof(storage));

    const uint8_t first[] = {1U, 2U, 3U, 4U};
    assert(USARTRxRingWrite(&ring, first, sizeof(first)) == sizeof(first));
    assert(USARTRxRingCount(&ring) == sizeof(first));

    uint8_t output[5] = {0};
    assert(USARTRxRingRead(&ring, output, 3U) == 3U);
    assert(output[0] == 1U);
    assert(output[1] == 2U);
    assert(output[2] == 3U);

    const uint8_t wrapped[] = {5U, 6U, 7U, 8U, 9U};
    assert(USARTRxRingWrite(&ring, wrapped, sizeof(wrapped)) == 4U);
    assert(ring.drop_count == 1U);
    assert(USARTRxRingCount(&ring) == sizeof(storage));

    assert(USARTRxRingRead(&ring, output, sizeof(output)) == sizeof(output));
    assert(output[0] == 4U);
    assert(output[1] == 5U);
    assert(output[2] == 6U);
    assert(output[3] == 7U);
    assert(output[4] == 8U);
    assert(USARTRxRingCount(&ring) == 0U);

    USARTRxRingReset(&ring);
    assert(ring.head == 0U);
    assert(ring.tail == 0U);
    assert(ring.count == 0U);
    assert(ring.drop_count == 0U);

    return 0;
}
