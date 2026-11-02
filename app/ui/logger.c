/**
 * @file    logger.c
 * @brief   Task 4 - Nhật ký truy cập có dấu thời gian (UC10).
 *          Định dạng: "[YYYY-MM-DD HH:MM:SS] UNLOCK_OK" / "UNLOCK_FAIL".
 */
#include "logger.h"

#include <stdio.h>

#include "queues.h"
#include "sw_rtc.h"
#include "uart_port.h"

void Logger_Init(void)
{
    sw_rtc_init();
}

void Logger_Process(void)
{
    LogEvt_t evt;
    while (Q_RecvLog(&evt)) {
        char ts[32];
        sw_rtc_format_timestamp(ts, sizeof(ts));
        char line[64];
        (void)snprintf(line, sizeof(line), "%s %s\r\n",
                       ts,
                       (evt == LOG_UNLOCK_OK) ? "UNLOCK_OK" : "UNLOCK_FAIL");
        uart_write(line);
    }
}
