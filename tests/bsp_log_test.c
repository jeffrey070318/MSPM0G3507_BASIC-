#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "bsp_log.h"

void SEGGER_RTT_Init(void)
{
}

int SEGGER_RTT_vprintf(unsigned int buffer_index, const char *format,
    va_list *args)
{
    (void) buffer_index;
    (void) format;
    (void) args;
    return 0;
}

int SEGGER_RTT_printf(unsigned int buffer_index, const char *format, ...)
{
    (void) buffer_index;
    (void) format;
    return 0;
}

int SEGGER_RTT_WriteString(unsigned int buffer_index, const char *text)
{
    (void) buffer_index;
    (void) text;
    return 0;
}

int main(void)
{
    char buffer[FLOAT2STR_BUFFER_SIZE];
    assert(Float2StrEx(buffer, sizeof(buffer), 1.5f) > 0);
    assert(strcmp(buffer, "1.500") == 0);

    assert(Float2StrEx(buffer, sizeof(buffer), -0.125f) > 0);
    assert(strcmp(buffer, "-0.125") == 0);

    char small[4];
    assert(Float2StrEx(small, sizeof(small), 12.5f) < 0);

    Float2Str(buffer, 1.005f);
    assert(strcmp(buffer, "1.005") == 0);
    return 0;
}
