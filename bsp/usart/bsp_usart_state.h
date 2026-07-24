#ifndef BSP_USART_STATE_H
#define BSP_USART_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    volatile bool active;
    volatile bool data_loaded;
    volatile bool eot_seen;
} USART_Tx_Completion_State_s;

typedef struct {
    uint8_t *buffer;
    uint16_t capacity;
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
    volatile uint32_t drop_count;
} USART_Rx_Ring_State_s;

void USARTTxCompletionBegin(USART_Tx_Completion_State_s *state);
void USARTTxCompletionReset(USART_Tx_Completion_State_s *state);
bool USARTTxCompletionOnDataLoaded(USART_Tx_Completion_State_s *state);
bool USARTTxCompletionOnEOT(USART_Tx_Completion_State_s *state);

void USARTRxRingInit(
    USART_Rx_Ring_State_s *state, uint8_t *buffer, uint16_t capacity);
void USARTRxRingReset(USART_Rx_Ring_State_s *state);
uint16_t USARTRxRingWrite(USART_Rx_Ring_State_s *state,
    const uint8_t *data, uint16_t size);
uint16_t USARTRxRingRead(
    USART_Rx_Ring_State_s *state, uint8_t *data, uint16_t capacity);
uint16_t USARTRxRingCount(const USART_Rx_Ring_State_s *state);

#endif
