#ifndef BSP_USART_STATE_H
#define BSP_USART_STATE_H

#include <stdbool.h>

typedef struct {
    volatile bool active;
    volatile bool data_loaded;
    volatile bool eot_seen;
} USART_Tx_Completion_State_s;

void USARTTxCompletionBegin(USART_Tx_Completion_State_s *state);
void USARTTxCompletionReset(USART_Tx_Completion_State_s *state);
bool USARTTxCompletionOnDataLoaded(USART_Tx_Completion_State_s *state);
bool USARTTxCompletionOnEOT(USART_Tx_Completion_State_s *state);

#endif
