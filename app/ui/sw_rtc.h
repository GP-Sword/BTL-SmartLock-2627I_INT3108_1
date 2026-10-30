/**
 * @file    sw_rtc.h
 * @brief   RTC phần mềm cho Task 4 (UC10).
 *          Giờ gốc từ __DATE__ / __TIME__ lúc build + app_now_ms() / 1000.
 */
#ifndef SW_RTC_H
#define SW_RTC_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} DateTime_t;

void sw_rtc_init(void);
void sw_rtc_get_datetime(DateTime_t *dt);
void sw_rtc_format_timestamp(char *buf, size_t maxlen);

#endif /* SW_RTC_H */
