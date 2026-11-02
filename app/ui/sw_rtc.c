/**
 * @file    sw_rtc.c
 * @brief   Cài đặt RTC phần mềm tính từ __DATE__ / __TIME__ + app_now_ms() / 1000.
 */
#include "sw_rtc.h"

#include <stdbool.h>
#include <stdio.h>

#include "app_time.h"

static uint32_t s_baseEpochSec = 0u;

static bool is_leap_year(uint16_t y)
{
    return ((y % 4u == 0u && y % 100u != 0u) || (y % 400u == 0u));
}

static uint8_t days_in_month(uint16_t y, uint8_t m)
{
    static const uint8_t dim[12] = { 31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u };
    if (m == 2u && is_leap_year(y)) {
        return 29u;
    }
    return (m >= 1u && m <= 12u) ? dim[m - 1u] : 30u;
}

static uint32_t date_to_days(uint16_t y, uint8_t m, uint8_t d)
{
    uint32_t days = 0u;
    for (uint16_t year = 1970u; year < y; year++) {
        days += is_leap_year(year) ? 366u : 365u;
    }
    for (uint8_t month = 1u; month < m; month++) {
        days += days_in_month(y, month);
    }
    days += (uint32_t)(d - 1u);
    return days;
}

static void days_to_date(uint32_t days, uint16_t *y, uint8_t *m, uint8_t *d)
{
    uint16_t year = 1970u;
    while (1) {
        uint16_t ydays = is_leap_year(year) ? 366u : 365u;
        if (days < ydays) {
            break;
        }
        days -= ydays;
        year++;
    }
    *y = year;

    uint8_t month = 1u;
    while (1) {
        uint8_t mdays = days_in_month(year, month);
        if (days < mdays) {
            break;
        }
        days -= mdays;
        month++;
    }
    *m = month;
    *d = (uint8_t)(days + 1u);
}

static uint8_t parse_month(const char *m)
{
    static const char * const months[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    for (uint8_t i = 0u; i < 12u; i++) {
        if (m[0] == months[i][0] && m[1] == months[i][1] && m[2] == months[i][2]) {
            return (uint8_t)(i + 1u);
        }
    }
    return 1u;
}

void sw_rtc_init(void)
{
    const char *date_str = __DATE__; /* "Mmm dd yyyy" */
    const char *time_str = __TIME__; /* "hh:mm:ss" */

    uint8_t month = parse_month(date_str);
    uint8_t day = (uint8_t)((date_str[4] == ' ' ? 0 : (date_str[4] - '0') * 10) +
                            (date_str[5] - '0'));
    uint16_t year = (uint16_t)((date_str[7] - '0') * 1000 +
                              (date_str[8] - '0') * 100 +
                              (date_str[9] - '0') * 10 +
                              (date_str[10] - '0'));

    uint8_t hour = (uint8_t)((time_str[0] - '0') * 10 + (time_str[1] - '0'));
    uint8_t min  = (uint8_t)((time_str[3] - '0') * 10 + (time_str[4] - '0'));
    uint8_t sec  = (uint8_t)((time_str[6] - '0') * 10 + (time_str[7] - '0'));

    s_baseEpochSec = date_to_days(year, month, day) * 86400u +
                     (uint32_t)hour * 3600u +
                     (uint32_t)min * 60u +
                     (uint32_t)sec;
}

void sw_rtc_get_datetime(DateTime_t *dt)
{
    if (dt == NULL) {
        return;
    }
    uint32_t now_sec = s_baseEpochSec + (app_now_ms() / 1000u);
    uint32_t days = now_sec / 86400u;
    uint32_t day_secs = now_sec % 86400u;

    days_to_date(days, &dt->year, &dt->month, &dt->day);
    dt->hour   = (uint8_t)(day_secs / 3600u);
    dt->minute = (uint8_t)((day_secs % 3600u) / 60u);
    dt->second = (uint8_t)(day_secs % 60u);
}

void sw_rtc_format_timestamp(char *buf, size_t maxlen)
{
    DateTime_t dt;
    sw_rtc_get_datetime(&dt);
    (void)snprintf(buf, maxlen, "[%04u-%02u-%02u %02u:%02u:%02u]",
                   dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
}
