#ifndef LINE_FOLLOW_H
#define LINE_FOLLOW_H

#include <stdbool.h>

typedef struct {
    float vx_mps;
    float wz_radps;
    bool line_valid;
    bool a_marker_event;
} LineFollow_Output_t;

bool LineFollowInit(void);
void LineFollowTask(bool enabled, float dt_seconds,
    LineFollow_Output_t *output);

#endif
