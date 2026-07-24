#ifndef TEST_INS_MESSAGE_CENTER_H
#define TEST_INS_MESSAGE_CENTER_H

#include <stdint.h>

typedef struct {
    const char *topic;
    uint8_t data_len;
} Subscriber_t;

typedef struct {
    const char *topic;
    uint8_t data_len;
} Publisher_t;

Subscriber_t *SubRegister(char *name, uint8_t data_len);
Publisher_t *PubRegister(char *name, uint8_t data_len);
uint8_t SubGetMessage(Subscriber_t *sub, void *data_ptr);
uint8_t PubPushMessage(Publisher_t *pub, void *data_ptr);

#endif
