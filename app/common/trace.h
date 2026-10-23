/**
 * @file    trace.h
 * @brief   TRACE("fmt", ...) in một dòng vết (tự thêm "\r\n") khi bật APP_TRACE.
 *
 * Bản release (không APP_TRACE): TRACE biến mất hoàn toàn, không tốn flash.
 * KHÔNG gọi TRACE trong ISR (uart_write có thể chờ UART).
 */
#ifndef TRACE_H
#define TRACE_H

#if defined(APP_TRACE) && !defined(UNIT_TEST)
void trace_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#define TRACE(...) trace_printf(__VA_ARGS__)
#else
#define TRACE(...) ((void)0)
#endif

#endif /* TRACE_H */
