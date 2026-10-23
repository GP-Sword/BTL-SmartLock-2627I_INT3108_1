/**
 * @file    app_time.h
 * @brief   Nguồn thời gian duy nhất cho logic ứng dụng.
 *
 * Firmware: app_now_ms() = HAL_GetTick() (cài trong app.c).
 * Unit test trên PC: test tự cài app_now_ms() bằng đồng hồ giả.
 * Logic thuần (Task 2, soft_timer) chỉ dùng hàm này, KHÔNG include HAL.
 */
#ifndef APP_TIME_H
#define APP_TIME_H

#include <stdint.h>

uint32_t app_now_ms(void);

/* Đúng cả khi tick tràn 32 bit (sau ~49 ngày). */
static inline uint32_t app_elapsed_ms(uint32_t since)
{
    return app_now_ms() - since;
}

#endif /* APP_TIME_H */
