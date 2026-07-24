#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "imu.h"
#include "ins.h"
#include "message_center.h"

typedef struct {
    Subscriber_t descriptor;
    bool has_message;
    uint8_t data[64];
} TestSubscriber_t;

typedef struct {
    Publisher_t descriptor;
    uint32_t push_count;
    uint8_t data[64];
} TestPublisher_t;

static TestSubscriber_t subscribers[2];
static TestPublisher_t publishers[1];
static uint8_t subscriber_count;
static uint8_t publisher_count;
static bool fail_registration;

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static TestSubscriber_t *FindSubscriber(const char *topic)
{
    for (uint8_t i = 0U; i < subscriber_count; ++i) {
        if (strcmp(subscribers[i].descriptor.topic, topic) == 0) {
            return &subscribers[i];
        }
    }
    return NULL;
}

static TestPublisher_t *FindPublisher(const char *topic)
{
    for (uint8_t i = 0U; i < publisher_count; ++i) {
        if (strcmp(publishers[i].descriptor.topic, topic) == 0) {
            return &publishers[i];
        }
    }
    return NULL;
}

static void FeedMessage(const char *topic, const void *data, uint8_t size)
{
    TestSubscriber_t *subscriber = FindSubscriber(topic);
    assert(subscriber != NULL);
    assert(subscriber->descriptor.data_len == size);
    memcpy(subscriber->data, data, size);
    subscriber->has_message = true;
}

Subscriber_t *SubRegister(char *name, uint8_t data_len)
{
    if (fail_registration || (subscriber_count >= 2U)) {
        return NULL;
    }
    TestSubscriber_t *subscriber = &subscribers[subscriber_count++];
    subscriber->descriptor.topic = name;
    subscriber->descriptor.data_len = data_len;
    subscriber->has_message = false;
    return &subscriber->descriptor;
}

Publisher_t *PubRegister(char *name, uint8_t data_len)
{
    if (fail_registration || (publisher_count >= 1U)) {
        return NULL;
    }
    TestPublisher_t *publisher = &publishers[publisher_count++];
    publisher->descriptor.topic = name;
    publisher->descriptor.data_len = data_len;
    publisher->push_count = 0U;
    return &publisher->descriptor;
}

uint8_t SubGetMessage(Subscriber_t *sub, void *data_ptr)
{
    TestSubscriber_t *subscriber = (TestSubscriber_t *) sub;
    if ((subscriber == NULL) || !subscriber->has_message ||
        (data_ptr == NULL)) {
        return 0U;
    }
    memcpy(data_ptr, subscriber->data, subscriber->descriptor.data_len);
    subscriber->has_message = false;
    return 1U;
}

uint8_t PubPushMessage(Publisher_t *pub, void *data_ptr)
{
    TestPublisher_t *publisher = (TestPublisher_t *) pub;
    if ((publisher == NULL) || (data_ptr == NULL)) {
        return 0U;
    }
    memcpy(publisher->data, data_ptr, publisher->descriptor.data_len);
    publisher->push_count++;
    return 1U;
}

int main(void)
{
    INS_StartReturn();
    assert(INS_GetState() == INS_STATE_IDLE);

    fail_registration = true;
    assert(!INS_Init());
    fail_registration = false;
    subscriber_count = 0U;
    publisher_count = 0U;
    assert(INS_Init());

    IMU_Data_t imu = {.yaw = 0.0f};
    Encoder_Pub_Data_t encoder = {
        .left_total = 100,
        .right_total = 100,
    };
    FeedMessage(INS_IMU_TOPIC, &imu, sizeof(imu));
    FeedMessage(INS_ENCODER_TOPIC, &encoder, sizeof(encoder));
    INS_Task(0.001f);
    INS_Position_t position = INS_GetPosition();
    assert(AbsFloat(position.x) < 0.00001f);
    assert(AbsFloat(position.y) < 0.00001f);

    encoder.left_total = 200;
    encoder.right_total = 200;
    FeedMessage(INS_ENCODER_TOPIC, &encoder, sizeof(encoder));
    INS_Task(0.001f);
    position = INS_GetPosition();
    assert(position.x > INS_TARGET_DISTANCE_M);
    assert(AbsFloat(position.y) < 0.001f);

    INS_ResetOrigin();
    position = INS_GetPosition();
    assert(AbsFloat(position.x) < 0.00001f);
    FeedMessage(INS_ENCODER_TOPIC, &encoder, sizeof(encoder));
    INS_Task(0.001f);
    position = INS_GetPosition();
    assert(AbsFloat(position.x) < 0.00001f);

    encoder.left_total = 300;
    encoder.right_total = 300;
    FeedMessage(INS_ENCODER_TOPIC, &encoder, sizeof(encoder));
    INS_Task(0.001f);
    position = INS_GetPosition();
    assert(position.x > INS_TARGET_DISTANCE_M);

    INS_StartReturn();
    imu.yaw = 180.0f;
    FeedMessage(INS_IMU_TOPIC, &imu, sizeof(imu));
    INS_Task(0.001f);
    TestPublisher_t *command_pub = FindPublisher(INS_CMD_TOPIC);
    assert(command_pub != NULL);
    INS_ChassisCommand_t command;
    memcpy(&command, command_pub->data, sizeof(command));
    assert(command.motion_enabled);
    assert(command.vx > 0.0f);
    assert(AbsFloat(command.wz) < 0.01f);

    encoder.left_total = 400;
    encoder.right_total = 400;
    FeedMessage(INS_ENCODER_TOPIC, &encoder, sizeof(encoder));
    INS_Task(0.001f);
    memcpy(&command, command_pub->data, sizeof(command));
    assert(!command.motion_enabled);
    assert(AbsFloat(command.vx) < 0.00001f);
    assert(AbsFloat(command.wz) < 0.00001f);
    assert(INS_GetState() == INS_STATE_DONE);
    assert(INS_CheckTargetReach() == INS_TARGET_REACHED);

    INS_ResetOrigin();
    imu.yaw = 180.0f;
    FeedMessage(INS_IMU_TOPIC, &imu, sizeof(imu));
    FeedMessage(INS_ENCODER_TOPIC, &encoder, sizeof(encoder));
    INS_Task(0.001f);
    encoder.left_total = 450;
    encoder.right_total = 450;
    FeedMessage(INS_ENCODER_TOPIC, &encoder, sizeof(encoder));
    INS_Task(0.001f);
    imu.yaw = 270.0f;
    encoder.left_total = 550;
    encoder.right_total = 550;
    FeedMessage(INS_IMU_TOPIC, &imu, sizeof(imu));
    FeedMessage(INS_ENCODER_TOPIC, &encoder, sizeof(encoder));
    INS_Task(0.001f);
    position = INS_GetPosition();
    assert(position.y > INS_TARGET_DISTANCE_M);
    assert(position.y > position.x);
    INS_StartReturn();
    INS_Task(0.001f);
    memcpy(&command, command_pub->data, sizeof(command));
    assert(command.motion_enabled);
    assert(command.wz > 1.0f);
    return 0;
}
