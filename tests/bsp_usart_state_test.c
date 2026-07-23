#include <assert.h>

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

    return 0;
}
