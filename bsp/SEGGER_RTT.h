#ifndef SEGGER_RTT_H
#define SEGGER_RTT_H

#include <stdarg.h>
#include <stdio.h>

#define RTT_CTRL_RESET ""
#define RTT_CTRL_CLEAR ""
#define RTT_CTRL_TEXT_BRIGHT_GREEN ""
#define RTT_CTRL_TEXT_BRIGHT_YELLOW ""
#define RTT_CTRL_TEXT_BRIGHT_RED ""

static inline void SEGGER_RTT_Init(void)
{
}

static inline int SEGGER_RTT_vprintf(
    unsigned BufferIndex, const char *sFormat, va_list *pParamList)
{
    (void) BufferIndex;
    return vprintf(sFormat, *pParamList);
}

static inline int SEGGER_RTT_printf(
    unsigned BufferIndex, const char *sFormat, ...)
{
    (void) BufferIndex;

    va_list args;
    va_start(args, sFormat);
    int ret = vprintf(sFormat, args);
    va_end(args);

    return ret;
}

static inline unsigned SEGGER_RTT_WriteString(
    unsigned BufferIndex, const char *s)
{
    (void) BufferIndex;
    return (unsigned) fputs(s, stdout);
}

#endif
