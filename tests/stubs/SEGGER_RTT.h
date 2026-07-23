#ifndef TEST_STUB_SEGGER_RTT_H
#define TEST_STUB_SEGGER_RTT_H

#include <stdarg.h>

#define RTT_CTRL_RESET ""
#define RTT_CTRL_CLEAR ""
#define RTT_CTRL_TEXT_BRIGHT_GREEN ""
#define RTT_CTRL_TEXT_BRIGHT_YELLOW ""
#define RTT_CTRL_TEXT_BRIGHT_RED ""

void SEGGER_RTT_Init(void);
int SEGGER_RTT_vprintf(unsigned int buffer_index, const char *format,
    va_list *args);
int SEGGER_RTT_printf(unsigned int buffer_index, const char *format, ...);
int SEGGER_RTT_WriteString(unsigned int buffer_index, const char *text);

#endif
