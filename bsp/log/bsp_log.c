#include "bsp_log.h"

#include "SEGGER_RTT.h"
#include "SEGGER_RTT_Conf.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>


void BSPLogInit(void)
{
    SEGGER_RTT_Init();
}

int PrintLog(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = SEGGER_RTT_vprintf(BUFFER_INDEX, fmt, &args); // 一次可以开启多个buffer(多个终端),我们只用一个
    va_end(args);
    return n;
}

void Float2Str(char *str, float va)
{
    (void) Float2StrEx(str, FLOAT2STR_BUFFER_SIZE, va);
}

int Float2StrEx(char *str, size_t str_size, float va)
{
    if ((str == NULL) || (str_size == 0U)) {
        return -1;
    }

    bool negative = va < 0.0f;
    float magnitude = negative ? -va : va;
    if (!(magnitude >= 0.0f) || (magnitude > 4294967.0f)) {
        str[0] = '\0';
        return -1;
    }

    uint32_t scaled = (uint32_t) (magnitude * 1000.0f + 0.5f);
    uint32_t head = scaled / 1000U;
    uint32_t point = scaled % 1000U;
    int written = snprintf(str, str_size, "%s%lu.%03lu",
        negative ? "-" : "", (unsigned long) head, (unsigned long) point);
    if ((written < 0) || ((size_t) written >= str_size)) {
        str[0] = '\0';
        return -1;
    }
    return written;
}
