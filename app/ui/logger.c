/**
 * @file    logger.c
 * @brief   STUB của foundation: in "[--] UNLOCK_OK" chưa có giờ, để scenario test
 *          bắt được chuỗi UNLOCK_OK/UNLOCK_FAIL ngay từ đầu.
 *          Task 4 thay bằng RTC phần mềm (sw_rtc.c): giờ gốc + HAL_GetTick()/1000.
 *          KHÔNG bật RTC phần cứng: Wokwi không mô phỏng RTC.
 */
#include "logger.h"

#include "queues.h"
#include "uart_port.h"

void Logger_Init(void)
{
    /* TODO(Task 4): sw_rtc_init() từ __DATE__/__TIME__ */
}

void Logger_Process(void)
{
    LogEvt_t evt;
    while (Q_RecvLog(&evt)) {
        /* TODO(Task 4): "[YYYY-MM-DD HH:MM:SS] ..." */
        uart_write((evt == LOG_UNLOCK_OK) ? "[--] UNLOCK_OK\r\n" : "[--] UNLOCK_FAIL\r\n");
    }
}
