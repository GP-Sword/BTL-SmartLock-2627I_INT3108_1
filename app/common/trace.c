/**
 * @file    trace.c
 * @brief   Cài đặt TRACE qua uart_write(). OWNER: foundation.
 */
#include "trace.h"

#if defined(APP_TRACE) && !defined(UNIT_TEST)

#include <stdarg.h>
#include <stdio.h>

#include "uart_port.h"

void trace_printf(const char *fmt, ...)
{
    char line[80];
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof(line) - 2u, fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
    }
    if ((unsigned)n > sizeof(line) - 3u) {
        n = (int)(sizeof(line) - 3u); /* cắt dòng quá dài */
    }
    line[n]     = '\r';
    line[n + 1] = '\n';
    line[n + 2] = '\0';
    uart_write(line);
}

#endif
