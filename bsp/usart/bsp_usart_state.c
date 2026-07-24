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

void USARTRxRingInit(
    USART_Rx_Ring_State_s *state, uint8_t *buffer, uint16_t capacity)
{
    if (state == NULL) {
        return;
    }

    state->buffer = buffer;
    state->capacity = capacity;
    USARTRxRingReset(state);
}

void USARTRxRingReset(USART_Rx_Ring_State_s *state)
{
    if (state == NULL) {
        return;
    }

    state->head = 0U;
    state->tail = 0U;
    state->count = 0U;
    state->drop_count = 0U;
}

uint16_t USARTRxRingWrite(USART_Rx_Ring_State_s *state,
    const uint8_t *data, uint16_t size)
{
    if ((state == NULL) || (state->buffer == NULL) ||
        (state->capacity == 0U) || (data == NULL)) {
        return 0U;
    }

    uint16_t written = 0U;
    while ((written < size) && (state->count < state->capacity)) {
        state->buffer[state->head] = data[written];
        state->head++;
        if (state->head >= state->capacity) {
            state->head = 0U;
        }
        state->count++;
        written++;
    }
    state->drop_count += (uint32_t) (size - written);
    return written;
}

uint16_t USARTRxRingRead(
    USART_Rx_Ring_State_s *state, uint8_t *data, uint16_t capacity)
{
    if ((state == NULL) || (state->buffer == NULL) ||
        (state->capacity == 0U) || (data == NULL)) {
        return 0U;
    }

    uint16_t read = 0U;
    while ((read < capacity) && (state->count > 0U)) {
        data[read] = state->buffer[state->tail];
        state->tail++;
        if (state->tail >= state->capacity) {
            state->tail = 0U;
        }
        state->count--;
        read++;
    }
    return read;
}

uint16_t USARTRxRingCount(const USART_Rx_Ring_State_s *state)
{
    if (state == NULL) {
        return 0U;
    }
    return state->count;
}
