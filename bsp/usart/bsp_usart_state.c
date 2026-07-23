#include "bsp_usart_state.h"

#include <stddef.h>

void USARTTxCompletionReset(USART_Tx_Completion_State_s *state)
{
    if (state == NULL) {
        return;
    }
    state->active = false;
    state->data_loaded = false;
    state->eot_seen = false;
}

void USARTTxCompletionBegin(USART_Tx_Completion_State_s *state)
{
    if (state == NULL) {
        return;
    }
    state->active = true;
    state->data_loaded = false;
    state->eot_seen = false;
}

bool USARTTxCompletionOnDataLoaded(USART_Tx_Completion_State_s *state)
{
    if ((state == NULL) || !state->active) {
        return false;
    }
    state->data_loaded = true;
    return state->eot_seen;
}

bool USARTTxCompletionOnEOT(USART_Tx_Completion_State_s *state)
{
    if ((state == NULL) || !state->active) {
        return false;
    }
    state->eot_seen = true;
    return state->data_loaded;
}
